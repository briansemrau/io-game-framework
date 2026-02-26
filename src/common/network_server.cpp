#include "network_server.h"

#include <chrono>
#include <cstring>
#include <iostream>
#include <mutex>

NetworkServer::NetworkServer(Game& game) : m_game(game) {}

NetworkServer::~NetworkServer() { stop(); }

void NetworkServer::start() {
    if (m_running.load()) {
        return;
    }
    m_running.store(true);
    m_networkThread = std::thread(&NetworkServer::run, this);
}

void NetworkServer::stop() {
    if (!m_running.load()) {
        return;
    }
    m_running.store(false);
    m_outgoingCv.notify_all();
    if (m_networkThread.joinable()) {
        m_networkThread.join();
    }
    std::lock_guard lock(m_clientsMutex);
    m_clients.clear();
}

bool NetworkServer::isRunning() const { return m_running.load(); }

void NetworkServer::send(ClientId clientId, const std::vector<std::byte>& data) {
    std::lock_guard lock(m_outgoingMutex);
    m_outgoingQueue.push({clientId, data, false});
    m_outgoingCv.notify_one();
}

void NetworkServer::broadcast(const std::vector<std::byte>& data) {
    std::lock_guard lock(m_outgoingMutex);
    m_outgoingQueue.push({0, data, true});
    m_outgoingCv.notify_one();
}

NetworkServer::ClientId NetworkServer::acceptConnection() {
    ClientId clientId = m_nextClientId.fetch_add(1);
    handleNewClient(clientId);
    return clientId;
}

void NetworkServer::run() {
    auto previousTime = std::chrono::steady_clock::now();
    auto remainingTime = Seconds::zero();
    while (m_running.load()) {
        const auto currentTime = std::chrono::steady_clock::now();
        const Seconds elapsed = std::min(Seconds(2), Seconds(currentTime - previousTime));
        previousTime = currentTime;
        remainingTime += elapsed;

        if (remainingTime >= m_tickPeriod) {
            processOutgoing();
            remainingTime -= m_tickPeriod;
        }

        // WebRTC networking events are processed by libdatachannel's internal thread.
        // Callbacks (onMessage, onDataChannel, etc.) are invoked from that thread and
        // are thread-safe due to the mutex protection in each callback handler.

        const auto sleepTime = std::min(m_tickPeriod - remainingTime, Seconds(1.0f));
        if (sleepTime.count() > 0.0f) {
            std::this_thread::sleep_for(sleepTime);
        }
    }
}

void NetworkServer::processOutgoing() {
    std::unique_lock lock(m_outgoingMutex);
    m_outgoingCv.wait_for(lock, std::chrono::milliseconds(10), [this] { return !m_outgoingQueue.empty() || !m_running.load(); });

    while (!m_outgoingQueue.empty()) {
        OutgoingMessage msg = m_outgoingQueue.front();
        m_outgoingQueue.pop();
        lock.unlock();

        std::lock_guard clientsLock(m_clientsMutex);
        if (msg.broadcast) {
            for (auto& [id, conn] : m_clients) {
                if (conn && conn->connected && conn->dataChannel) {
                    conn->dataChannel->send(reinterpret_cast<const std::byte*>(msg.data.data()), msg.data.size());
                }
            }
        } else {
            auto it = m_clients.find(msg.clientId);
            if (it != m_clients.end() && it->second && it->second->connected && it->second->dataChannel) {
                it->second->dataChannel->send(reinterpret_cast<const std::byte*>(msg.data.data()), msg.data.size());
            }
        }

        lock.lock();
    }
}

void NetworkServer::handleNewClient(ClientId clientId) {
    auto conn = std::make_unique<ClientConnection>();

    rtc::Configuration config;

    // TODO

    conn->peerConnection = std::make_shared<rtc::PeerConnection>(config);

    {
        std::lock_guard lock(m_clientsMutex);
        m_clients[clientId] = std::move(conn);
    }

    setupPeerConnectionCallbacks(clientId);

    rtc::DataChannelInit dcConfig;
    dcConfig.negotiated = true;
    dcConfig.id = 0;
    {
        std::lock_guard lock(m_clientsMutex);
        auto it = m_clients.find(clientId);
        if (it != m_clients.end() && it->second->peerConnection) {
            it->second->dataChannel = it->second->peerConnection->createDataChannel("game", dcConfig);
        }
    }

    setupDataChannelCallbacks(clientId);

    std::lock_guard lock(m_clientsMutex);
    auto it = m_clients.find(clientId);
    if (it != m_clients.end() && it->second->peerConnection) {
        auto localDescription = it->second->peerConnection->localDescription().value();
        it->second->localDescription = std::string(localDescription);
    }
}

void NetworkServer::setupPeerConnectionCallbacks(ClientId clientId) {
    std::lock_guard lock(m_clientsMutex);
    auto it = m_clients.find(clientId);
    if (it == m_clients.end() || !it->second->peerConnection) {
        return;
    }
    auto& conn = *it->second;

    conn.peerConnection->onLocalDescription([this, clientId](const rtc::Description& desc) {
        std::lock_guard lock(m_clientsMutex);
        auto it = m_clients.find(clientId);
        if (it != m_clients.end()) {
            it->second->localDescription = std::string(desc);
        }
    });

    conn.peerConnection->onLocalCandidate([this, clientId](const rtc::Candidate& cand) {});

    conn.peerConnection->onStateChange([this, clientId](rtc::PeerConnection::State state) {
        if (state == rtc::PeerConnection::State::Disconnected || state == rtc::PeerConnection::State::Failed) {
            std::lock_guard lock(m_clientsMutex);
            auto it = m_clients.find(clientId);
            if (it != m_clients.end()) {
                it->second->connected = false;
            }

        }
    });

    conn.peerConnection->onDataChannel([this, clientId](std::shared_ptr<rtc::DataChannel> dataChannel) {
        std::lock_guard lock(m_clientsMutex);
        auto it = m_clients.find(clientId);
        if (it != m_clients.end()) {
            it->second->dataChannel = dataChannel;
            setupDataChannelCallbacks(clientId);
        }
    });
}

void NetworkServer::setupDataChannelCallbacks(ClientId clientId) {
    // std::shared_ptr<rtc::DataChannel> dataChannel;
    // {
    //     std::lock_guard lock(m_clientsMutex);
    //     auto it = m_clients.find(clientId);
    //     if (it == m_clients.end()) {
    //         return;
    //     }
    //     dataChannel = it->second->dataChannel;
    // }

    // if (!dataChannel) {
    //     return;
    // }

    // dataChannel->onOpen([this, clientId]() {
    //     std::lock_guard lock(m_clientsMutex);
    //     auto it = m_clients.find(clientId);
    //     if (it != m_clients.end()) {
    //         it->second->connected = true;
    //     }
    //     std::lock_guard cbLock(m_callbackMutex);
    //     if (m_connectCallback) {
    //         m_connectCallback(clientId);
    //     }
    // });

    // dataChannel->onMessage([this, clientId](const std::variant<rtc::Binary, std::string>& msg) {
    //     std::vector<std::byte> data;
    //     if (std::holds_alternative<rtc::Binary>(msg)) {
    //         const auto& bin = std::get<rtc::Binary>(msg);
    //         data.resize(bin.size());
    //         std::memcpy(data.data(), bin.data(), bin.size());
    //     } else {
    //         const auto& str = std::get<std::string>(msg);
    //         data.resize(str.size());
    //         std::memcpy(data.data(), str.data(), str.size());
    //     }
    //     std::lock_guard cbLock(m_callbackMutex);
    //     if (m_messageCallback) {
    //         m_messageCallback(clientId, data);
    //     }
    // });

    // dataChannel->onClosed([this, clientId]() {
    //     {
    //         std::lock_guard lock(m_clientsMutex);
    //         auto it = m_clients.find(clientId);
    //         if (it != m_clients.end()) {
    //             it->second->connected = false;
    //             m_clients.erase(it);
    //         }
    //     }
    //     std::lock_guard cbLock(m_callbackMutex);
    //     if (m_disconnectCallback) {
    //         m_disconnectCallback(clientId);
    //     }
    // });
}

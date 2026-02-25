#include "network_server.h"

#include <chrono>
#include <cstring>
#include <iostream>
#include <mutex>

NetworkServer::NetworkServer(Config config) : m_config(std::move(config)) {}

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

std::string NetworkServer::getLocalDescription(ClientId clientId) {
    std::lock_guard lock(m_clientsMutex);
    auto it = m_clients.find(clientId);
    if (it == m_clients.end()) {
        return "";
    }
    return it->second->localDescription;
}

void NetworkServer::setRemoteDescription(ClientId clientId, const std::string& sdp) {
    std::lock_guard lock(m_clientsMutex);
    auto it = m_clients.find(clientId);
    if (it == m_clients.end()) {
        return;
    }
    if (!it->second->peerConnection) {
        return;
    }
    it->second->peerConnection->setRemoteDescription(rtc::Description(sdp));
}

void NetworkServer::setRemoteCandidate(ClientId clientId, const std::string& candidate, const std::string& mid) {
    std::lock_guard lock(m_clientsMutex);
    auto it = m_clients.find(clientId);
    if (it == m_clients.end()) {
        return;
    }
    if (!it->second->peerConnection) {
        return;
    }
    it->second->peerConnection->addRemoteCandidate(rtc::Candidate(candidate, mid));
}

void NetworkServer::setMessageCallback(MessageCallback callback) {
    std::lock_guard lock(m_callbackMutex);
    m_messageCallback = std::move(callback);
}

void NetworkServer::setConnectCallback(ConnectCallback callback) {
    std::lock_guard lock(m_callbackMutex);
    m_connectCallback = std::move(callback);
}

void NetworkServer::setDisconnectCallback(DisconnectCallback callback) {
    std::lock_guard lock(m_callbackMutex);
    m_disconnectCallback = std::move(callback);
}

void NetworkServer::run() {
    auto previousTime = std::chrono::steady_clock::now();
    auto remainingTime = Seconds::zero();
    while (m_running.load()) {
        // The stream of time
        const auto currentTime = std::chrono::steady_clock::now();
        const Seconds elapsed = std::min(Seconds(2), Seconds(currentTime - previousTime));  // don't handle major time skips
        previousTime = currentTime;
        remainingTime += elapsed;

        if (remainingTime >= m_tickPeriod) {

            processOutgoing();
            remainingTime -= m_tickPeriod;
        }

        // TODO queue networking work

        // And sleep
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
                if (conn->connected && conn->dataChannel) {
                    conn->dataChannel->send(rtc::DataChannel::Message(reinterpret_cast<const std::byte*>(msg.data.data()), msg.data.size()));
                }
            }
        } else {
            auto it = m_clients.find(msg.clientId);
            if (it != m_clients.end() && it->second->connected && it->second->dataChannel) {
                it->second->dataChannel->send(rtc::DataChannel::Message(reinterpret_cast<const std::byte*>(msg.data.data()), msg.data.size()));
            }
        }

        lock.lock();
    }
}

void NetworkServer::handleNewClient(ClientId clientId) {
    std::lock_guard lock(m_clientsMutex);
    auto conn = std::make_unique<ClientConnection>();

    rtc::Configuration config;
    for (const auto& server : m_config.iceServers) {
        config.iceServers.push_back(rtc::IceServer(server));
    }
    if (m_config.portRangeBegin > 0 && m_config.portRangeEnd > 0) {
        config.portRangeBegin = m_config.portRangeBegin;
        config.portRangeEnd = m_config.portRangeEnd;
    }

    conn->peerConnection = std::make_shared<rtc::PeerConnection>(config);
    setupPeerConnectionCallbacks(clientId, *conn);

    rtc::DataChannelInit dcConfig;
    dcConfig.negotiated = true;
    dcConfig.id = 0;
    conn->dataChannel = conn->peerConnection->createDataChannel("game", dcConfig);
    setupDataChannelCallbacks(clientId, *conn);

    auto localDescription = conn->peerConnection->localDescription().value();
    conn->localDescription = std::string(localDescription);

    m_clients[clientId] = std::move(conn);
}

void NetworkServer::setupPeerConnectionCallbacks(ClientId clientId, ClientConnection& conn) {
    if (!conn.peerConnection) {
        return;
    }

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
            std::lock_guard cbLock(m_callbackMutex);
            if (m_disconnectCallback) {
                m_disconnectCallback(clientId);
            }
        }
    });
}

void NetworkServer::setupDataChannelCallbacks(ClientId clientId, ClientConnection& conn) {
    if (!conn.dataChannel) {
        return;
    }

    conn.dataChannel->onOpen([this, clientId]() {
        std::lock_guard lock(m_clientsMutex);
        auto it = m_clients.find(clientId);
        if (it != m_clients.end()) {
            it->second->connected = true;
        }
        std::lock_guard cbLock(m_callbackMutex);
        if (m_connectCallback) {
            m_connectCallback(clientId);
        }
    });

    conn.dataChannel->onMessage([this, clientId](const std::variant<rtc::Binary, std::string>& msg) {
        std::vector<std::byte> data;
        if (std::holds_alternative<rtc::Binary>(msg)) {
            const auto& bin = std::get<rtc::Binary>(msg);
            data.resize(bin.size());
            std::memcpy(data.data(), bin.data(), bin.size());
        } else {
            const auto& str = std::get<std::string>(msg);
            data.resize(str.size());
            std::memcpy(data.data(), str.data(), str.size());
        }
        std::lock_guard cbLock(m_callbackMutex);
        if (m_messageCallback) {
            m_messageCallback(clientId, data);
        }
    });

    conn.dataChannel->onClosed([this, clientId]() {
        std::lock_guard lock(m_clientsMutex);
        auto it = m_clients.find(clientId);
        if (it != m_clients.end()) {
            it->second->connected = false;
        }
        std::lock_guard cbLock(m_callbackMutex);
        if (m_disconnectCallback) {
            m_disconnectCallback(clientId);
        }
    });
}

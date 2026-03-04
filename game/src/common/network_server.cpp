#include "network_server.h"

#include <plog/Log.h>

#include <chrono>
#include <future>
#include <mutex>
#include <nlohmann/json.hpp>

NetworkServer::NetworkServer(const Game& game) : m_game(game) {}

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
    if (m_networkThread.joinable()) {
        m_networkThread.join();
    }
    std::lock_guard lock(m_clientsMutex);
    m_clients.clear();
}

bool NetworkServer::isRunning() const { return m_running.load(); }

void NetworkServer::startSignallingWebsocket(const std::string& signalServerUrl, const uint16_t port) {
    m_localID = 12345;  // TODO

    auto ws = std::make_shared<rtc::WebSocket>();
    std::promise<void> wsPromise;
    auto wsFuture = wsPromise.get_future();

    ws->onOpen([&wsPromise, localID = m_localID, ws]() {
        nlohmann::json registerMsg = { { "id", std::to_string(localID) }, { "type", "register" }, { "role", "server" }, { "name", "Server " + std::to_string(localID) },
            { "max_players", 32 }, { "game_mode", "default" } };
        ws->send(registerMsg.dump());
        wsPromise.set_value();
    });

    ws->onError([&wsPromise, localID = m_localID](std::string s) {
        PLOG_ERROR << "WebSocket error for server (localID=" << localID << "): " << s;
        wsPromise.set_exception(std::make_exception_ptr(std::runtime_error(s)));
    });

    ws->onClosed([localID = m_localID]() { PLOG_DEBUG << "WebSocket closed for server (localID=" << localID << ")"; });

    ws->onMessage([this, wws = std::weak_ptr<rtc::WebSocket>(ws)](auto data) {
        if (!std::holds_alternative<std::string>(data)) {
            return;
        }

        const std::string rawData = std::get<std::string>(data);

        nlohmann::json message;
        try {
            message = nlohmann::json::parse(rawData);
        } catch (const std::exception& e) {
            PLOG_ERROR << "Failed to parse JSON: " << e.what();
            return;
        }

        handleSignallingMessage(message);
    });

    m_signallingWebsocket = ws;

    const std::string url =
        (signalServerUrl.find("://") == std::string::npos ? "ws://" : "") + signalServerUrl + ":" + std::to_string(port) + "/connect/" + std::to_string(m_localID);
    PLOG_DEBUG << "Opening WebSocket to: " << url;
    ws->open(url);
    PLOG_DEBUG << "WebSocket open() called, waiting for connection...";
    if (wsFuture.wait_for(std::chrono::seconds(90)) == std::future_status::timeout) {
        PLOG_ERROR << "WebSocket connection timeout after 90 seconds";
        return;
    }
}

void NetworkServer::handleSignallingMessage(const nlohmann::json& message) {
    auto idIt = message.find("id");
    if (idIt == message.end()) {
        PLOG_ERROR << "Message missing 'id' field";
        return;
    }

    PeerID id;
    try {
        id = std::stoull(idIt->get<std::string>());
    } catch (const std::invalid_argument&) {
        PLOG_ERROR << "Invalid peer ID in message";
        return;
    }

    auto typeIt = message.find("type");
    if (typeIt == message.end()) {
        PLOG_ERROR << "Message missing 'type' field";
        return;
    }
    const std::string type = typeIt->get<std::string>();

    std::shared_ptr<rtc::PeerConnection> pc;
    if (auto jt = m_clients.find(id); jt != m_clients.end()) {
        pc = jt->second.peerConnection;
    } else if (type == "offer" || type == "join") {
        if (type == "join") {
            // TODO reject clients if game is not accepting new joins
            if (false) {
                PLOG_ERROR << "Rejecting join for peer ID " << id;
            }
        }
        createClientConnection(id);
        auto kt = m_clients.find(id);
        if (kt != m_clients.end()) {
            pc = kt->second.peerConnection;
        } else {
            PLOG_ERROR << "Failed to find newly created connection";
            return;
        }
    } else {
        PLOG_ERROR << "No connection exists for peer " << id << " and type is " << type;
        return;
    }

    if (type == "offer" || type == "answer") {
        auto sdp = message["description"].get<std::string>();
        pc->setRemoteDescription(rtc::Description(sdp, type));
    } else if (type == "candidate") {
        auto candidate = message["candidate"].get<std::string>();
        auto mid = message["mid"].get<std::string>();
        pc->addRemoteCandidate(rtc::Candidate(candidate, mid));
    }
}

void NetworkServer::createClientConnection(const PeerID id) {
    PLOG_DEBUG << "Creating connection for peer ID " << id;
    assert(!m_clients.contains(id));

    rtc::Configuration config;
    for (const auto& url : getDefaultIceServerUrls()) {
        config.iceServers.push_back(rtc::IceServer(url));
    }
    // config.enableIceUdpMux = true;

    NetworkConnection clientConn;
    clientConn.peerConnection = std::make_shared<rtc::PeerConnection>(config);
    auto pc = clientConn.peerConnection;

    pc->onStateChange([id](rtc::PeerConnection::State state) { PLOG_DEBUG << "Peer " << id << " state changed to: " << state; });

    pc->onGatheringStateChange([id](rtc::PeerConnection::GatheringState state) { PLOG_DEBUG << "Peer " << id << " ICE gathering state: " << state; });

    pc->onLocalDescription([ws = m_signallingWebsocket, id](rtc::Description description) {
        PLOG_DEBUG << "Peer " << id << " generated local " << description.typeString() << " (" << std::string(description).size() << " bytes)";
        nlohmann::json message = { { "id", std::to_string(id) }, { "type", description.typeString() }, { "description", std::string(description) } };
        if (ws) {
            PLOG_DEBUG << "Sending " << description.typeString() << " via websocket";
            ws->send(message.dump());
        } else {
            PLOG_ERROR << "WebSocket is closed, cannot send " << description.typeString();
        }
    });

    pc->onLocalCandidate([ws = m_signallingWebsocket, id](rtc::Candidate candidate) {
        nlohmann::json message = { { "id", std::to_string(id) }, { "type", "candidate" }, { "candidate", std::string(candidate) }, { "mid", candidate.mid() } };
        if (ws) {
            ws->send(message.dump());
        } else {
            PLOG_ERROR << "WebSocket is closed, cannot send candidate";
        }
    });

    // Inbound data channels (test only)
    pc->onDataChannel([this, id](std::shared_ptr<rtc::DataChannel> dc) {
        PLOG_DEBUG << "Peer " << id << " opened data channel with label: " << dc->label();

        if (dc->label() == TEST_DATACHANNEL) {
            PLOG_DEBUG << "Attaching test data channel handler for peer " << id;
            std::lock_guard lock(m_clientsMutex);
            if (auto it = m_clients.find(id); it != m_clients.end()) {
                it->second.testDataChannel = dc;
            }
            dc->onOpen([id, wdc = std::weak_ptr<rtc::DataChannel>(dc)]() { PLOG_INFO << "Test datachannel opened for server " << id; });
            dc->onClosed([id]() { PLOG_INFO << "Test datachannel closed for server " << id; });
            dc->onMessage([this, id, wdc = std::weak_ptr<rtc::DataChannel>(dc)](auto data) {
                if (std::holds_alternative<rtc::binary>(data)) {
                    auto& binaryData = std::get<rtc::binary>(data);
                    PLOG_DEBUG << "Received test message from server " << id << " (" << binaryData.size() << " bytes)";
                }
            });
        } else {
            PLOG_DEBUG << "Unexpected data channel label: " << dc->label();
        }
    });

    // Outbound data channels (everything from server)
    {
        auto dcinit = rtc::DataChannelInit{
            .reliability =
                rtc::Reliability{
                    .unordered = true,
                    .maxRetransmits = 0,
                },
        };
        clientConn.stateDataChannel = pc->createDataChannel(STATE_DATACHANNEL, dcinit);
        auto& dc = clientConn.stateDataChannel;
        dc->onOpen([localID = m_localID, id, wdc = std::weak_ptr<rtc::DataChannel>(dc)]() {
            PLOG_DEBUG << "DataChannel from " << id << " open";
            if (auto openedDc = wdc.lock()) {
                openedDc->send("Hello from server " + std::to_string(localID));
                PLOG_DEBUG << "Sent hello message to client " << id;
            }
        });
        dc->onClosed([id]() { PLOG_DEBUG << "DataChannel from " << id << " closed"; });
        dc->onMessage([id](auto data) {
            if (std::holds_alternative<std::string>(data))
                PLOG_DEBUG << "Message from " << id << " received: " << std::get<std::string>(data);
            else
                PLOG_DEBUG << "Binary message from " << id << " received, size=" << std::get<rtc::binary>(data).size();
        });
    }
    {
        clientConn.testDataChannel = pc->createDataChannel(TEST_DATACHANNEL, {});
        auto& dc = clientConn.testDataChannel;
        dc->onOpen([localID = m_localID, id, wdc = std::weak_ptr<rtc::DataChannel>(dc)]() {
            PLOG_DEBUG << "DataChannel from " << id << " open";
            if (auto openedDc = wdc.lock()) {
                openedDc->send("Hello from server " + std::to_string(localID));
                PLOG_DEBUG << "Sent hello message to client " << id;
            }
        });
        dc->onClosed([id]() { PLOG_DEBUG << "DataChannel from " << id << " closed"; });
        dc->onMessage([id](auto data) {
            if (std::holds_alternative<std::string>(data))
                PLOG_DEBUG << "Message from " << id << " received: " << std::get<std::string>(data);
            else
                PLOG_DEBUG << "Binary message from " << id << " received, size=" << std::get<rtc::binary>(data).size();
        });
    }

    // Store the connection in our clients map
    {
        std::lock_guard lock(m_clientsMutex);
        m_clients[id] = std::move(clientConn);
        PLOG_DEBUG << "Stored connection in m_clients, total clients: " << m_clients.size();
    }

    PLOG_DEBUG << "Successfully created server connection for peer ID " << id;
}

void NetworkServer::run() {
    PLOG_DEBUG << "Starting";

#pragma warning(push)
#pragma warning(disable : 4996)
    auto signallingUrlCStr = std::getenv("SIGNALING_URL");
#pragma warning(pop)
    std::string signallingUrl = "localhost:9812";
    if (signallingUrlCStr) {
        signallingUrl = signallingUrlCStr;
    }
    PLOG_DEBUG << "SIGNALING_URL env=" << (signallingUrlCStr ? signallingUrl : "(not set, using default)") << ", using URL: " << signallingUrl;

    size_t colonPos = signallingUrl.find(':');
    std::string host = signallingUrl.substr(0, colonPos);
    uint16_t port = static_cast<uint16_t>(std::stoi(signallingUrl.substr(colonPos + 1)));

    startSignallingWebsocket(host, port);

    auto previousTime = std::chrono::steady_clock::now();
    auto remainingTime = Seconds::zero();
    PLOG_DEBUG << "Main network loop started (tick period=" << m_tickPeriod.count() << "s)";
    while (m_running.load()) {
        const auto currentTime = std::chrono::steady_clock::now();
        const Seconds elapsed = std::min(Seconds(2), Seconds(currentTime - previousTime));
        previousTime = currentTime;
        remainingTime += elapsed;

        if (remainingTime >= m_tickPeriod) {
            // TODO copy game state, ...
            remainingTime -= m_tickPeriod;
        }

        const auto sleepTime = std::min(m_tickPeriod - remainingTime, Seconds(1.0f));
        if (sleepTime.count() > 0.0f) {
            std::this_thread::sleep_for(sleepTime);
        }
    }
    PLOG_DEBUG << "Terminating";
}

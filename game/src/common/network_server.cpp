#include "network_server.h"

#include <plog/Log.h>

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <mutex>
#include <nlohmann/json.hpp>

NetworkServer::NetworkServer(Game& game) : m_game(game) { PLOG_DEBUG << "Constructor called"; }

NetworkServer::~NetworkServer() {
    stop();
}

void NetworkServer::start() {
    if (m_running.load()) {
        PLOG_DEBUG << "Already running, returning";
        return;
    }
    m_running.store(true);
    PLOG_DEBUG << "Spawning network thread";
    m_networkThread = std::thread(&NetworkServer::run, this);
    PLOG_DEBUG << "Network thread spawned successfully";
}

void NetworkServer::stop() {
    if (!m_running.load()) {
        PLOG_DEBUG << "Not running, returning";
        return;
    }
    m_running.store(false);
    if (m_networkThread.joinable()) {
        PLOG_DEBUG << "Joining network thread...";
        m_networkThread.join();
        PLOG_DEBUG << "Network thread joined";
    }
    std::lock_guard lock(m_clientsMutex);
    PLOG_DEBUG << "Clearing " << m_clients.size() << " client connections";
    m_clients.clear();
    PLOG_DEBUG << "Completed";
}

bool NetworkServer::isRunning() const { return m_running.load(); }

// void NetworkServer::startSignaling(uint16_t port) {
//     rtc::Configuration config;

//     // TODO put this in some config file
//     const std::vector<std::string> iceServerURLs{
//         {"stun:stun.l.google.com:19302"}, {"stun:stun.l.google.com:5349"},  {"stun:stun1.l.google.com:3478"}, {"stun:stun1.l.google.com:5349"},
//         {"stun:stun2.l.google.com:19302"},
//         {"stun:stun2.l.google.com:5349"}, {"stun:stun3.l.google.com:3478"}, {"stun:stun3.l.google.com:5349"}, {"stun:stun4.l.google.com:19302"},
//         {"stun:stun4.l.google.com:5349"}};
//     for (const auto& server : iceServerURLs) {
//         config.iceServers.push_back(rtc::IceServer(server));
//     }

//     config.enableIceUdpMux = true;
//     // m_localID = randomId(32);
//     // PLOG_DEBUG << "Local ID: " << m_localID;

//     auto ws = std::make_shared<rtc::WebSocket>();

//     std::promise<void> wsPromise;
//     auto wsFuture = wsPromise.get_future();

//     ws->onOpen([&wsPromise]() {
//         PLOG_DEBUG << "Websocket connected; ready to signal";
//         wsPromise.set_value();
//     });
//     ws->onError([&wsPromise](std::string s) {
//         PLOG_DEBUG << "Websocket error";
//         wsPromise.set_exception(std::make_exception_ptr(std::runtime_error(s)));
//     });
//     ws->onClosed([]() { PLOG_DEBUG << "Websocket closed"; });

//     ws->onMessage([this, &config, wws = static_cast<std::weak_ptr<rtc::WebSocket>>(ws)](auto data) {
//         if (!std::holds_alternative<std::string>(data)) return;

//         nlohmann::json message = nlohmann::json::parse(std::get<std::string>(data));

//         auto it = message.find("id");
//         if (it == message.end()) return;
//         auto id = it->get<std::string>();

//         it = message.find("type");
//         if (it == message.end()) return;
//         auto type = it->get<std::string>();

//         if (m_peerConnection != nullptr) {
//             // do nothing
//         } else if (!m_peerConnection && type == "offer") {
//             PLOG_DEBUG << "Answering to " << id;
//             createPeerConnection(config, wws, id);
//         } else {
//             PLOG_DEBUG << "Peer connection already exists and type is " << type << ". How did we get here?";
//             return;
//         }

//         if (type == "offer" || type == "answer") {
//             auto sdp = message["description"].get<std::string>();
//             m_peerConnection->setRemoteDescription(rtc::Description(sdp, type));
//         } else if (type == "candidate") {
//             auto sdp = message["candidate"].get<std::string>();
//             auto mid = message["mid"].get<std::string>();
//             m_peerConnection->addRemoteCandidate(rtc::Candidate(sdp, mid));
//         }
//     });

//     const std::string url = (serverUrl.find("://") == std::string::npos ? "ws://" : "") + serverUrl + ":" + std::to_string(serverPort) + "/" + m_localID;
//     ws->open(url);
//     PLOG_DEBUG << "Waiting for websocket to connect...";
// }

void NetworkServer::startSignallingWebsocket(const std::string& signalServerUrl, uint16_t port) {
    PLOG_DEBUG << "Called with signalServerUrl=" << signalServerUrl << " port=" << port;

    rtc::Configuration config;
    PLOG_DEBUG << "Created rtc::Configuration";

    // TODO put this in some config file
    const std::vector<std::string> iceServerURLs{
        {"stun:stun.l.google.com:19302"}, {"stun:stun.l.google.com:5349"},  {"stun:stun1.l.google.com:3478"}, {"stun:stun1.l.google.com:5349"},  {"stun:stun2.l.google.com:19302"},
        {"stun:stun2.l.google.com:5349"}, {"stun:stun3.l.google.com:3478"}, {"stun:stun3.l.google.com:5349"}, {"stun:stun4.l.google.com:19302"}, {"stun:stun4.l.google.com:5349"}};
    PLOG_DEBUG << "Adding " << iceServerURLs.size() << " ICE servers";
    for (const auto& server : iceServerURLs) {
        config.iceServers.push_back(rtc::IceServer(server));
        PLOG_DEBUG << "Added ICE server: " << server;
    }

    config.enableIceUdpMux = true;
    PLOG_DEBUG << "Enabled ICE UDP mux";

    PeerID localID = generateRandomPeerID();
    PLOG_DEBUG << "Generated local PeerID: " << localID;

    auto ws = std::make_shared<rtc::WebSocket>();
    PLOG_DEBUG << "Created WebSocket object";

    std::promise<void> wsPromise;
    auto wsFuture = wsPromise.get_future();

    ws->onOpen([&wsPromise, localID]() {
        PLOG_DEBUG << "WebSocket connected for server (localID=" << localID << ")";
        wsPromise.set_value();
    });
    ws->onError([&wsPromise, localID](std::string s) {
        PLOG_ERROR << "[WebSocket.onError] WebSocket error for server (localID=" << localID << "): " << s;
        wsPromise.set_exception(std::make_exception_ptr(std::runtime_error(s)));
    });
    ws->onClosed([localID]() { PLOG_DEBUG << "WebSocket closed for server (localID=" << localID << ")"; });

    ws->onMessage([this, &config, wws = static_cast<std::weak_ptr<rtc::WebSocket>>(ws)](auto data) {
        if (!std::holds_alternative<std::string>(data)) {
            PLOG_DEBUG << "Received non-string message, ignoring";
            return;
        }

        const std::string rawData = std::get<std::string>(data);
        PLOG_DEBUG << "Received message (" << rawData.size() << " bytes): " << rawData.substr(0, std::min(rawData.size(), size_t(200)));

        nlohmann::json message;
        try {
            message = nlohmann::json::parse(rawData);
            PLOG_DEBUG << "Parsed JSON successfully";
        } catch (const std::exception& e) {
            PLOG_ERROR << "[WebSocket.onMessage] Failed to parse JSON: " << e.what();
            return;
        }

        auto it = message.find("id");
        if (it == message.end()) {
            PLOG_DEBUG << "Message missing 'id' field";
            return;
        }
        auto id_str = it->get<std::string>();
        PLOG_DEBUG << "Message 'id' field: " << id_str;

        PeerID id;
        try {
            id = std::stoull(id_str);
            PLOG_DEBUG << "Parsed peer ID: " << id;
        } catch (const std::invalid_argument& ia) {
            PLOG_ERROR << "[WebSocket.onMessage] Message peer ID is invalid: " << ia.what() << " (value: " << id_str << ")";
            return;
        }

        it = message.find("type");
        if (it == message.end()) {
            PLOG_DEBUG << "Message missing 'type' field";
            return;
        }
        auto type = it->get<std::string>();
        PLOG_DEBUG << "Message 'type' field: " << type;

        std::shared_ptr<rtc::PeerConnection> pc;
        if (auto jt = m_clients.find(id); jt != m_clients.end()) {
            PLOG_DEBUG << "Found existing client connection for peer ID " << id;
            pc = jt->second->peerConnection;
        } else if (type == "offer") {
            PLOG_DEBUG << "Received 'offer' from peer " << id << ", creating new peer connection";
            createPeerConnection(config, wws, id);
            // Get the newly created connection
            auto jt = m_clients.find(id);
            if (jt != m_clients.end()) {
                pc = jt->second->peerConnection;
            } else {
                PLOG_ERROR << "[WebSocket.onMessage] Failed to find newly created peer connection";
                return;
            }
        } else {
            PLOG_ERROR << "[WebSocket.onMessage] No peer connection exists for peer " << id << " and type is " << type;
            return;
        }

        if (type == "offer" || type == "answer") {
            PLOG_DEBUG << "Processing SDP " << type << " for peer " << id;
            auto sdp = message["description"].get<std::string>();
            PLOG_DEBUG << "SDP " << type << " length: " << sdp.size() << " bytes";
            pc->setRemoteDescription(rtc::Description(sdp, type));
            PLOG_DEBUG << "Set remote " << type << " successfully";
        } else if (type == "candidate") {
            PLOG_DEBUG << "Processing ICE candidate for peer " << id;
            auto sdp = message["candidate"].get<std::string>();
            auto mid = message["mid"].get<std::string>();
            PLOG_DEBUG << "ICE candidate mid=" << mid << " sdp_length=" << sdp.size();
            pc->addRemoteCandidate(rtc::Candidate(sdp, mid));
            PLOG_DEBUG << "Added remote candidate successfully";
        } else {
            PLOG_DEBUG << "Unknown message type: " << type;
        }
    });

    m_signallingWebsocket = ws;

    const std::string url =
        (signalServerUrl.find("://") == std::string::npos ? "ws://" : "") + signalServerUrl + ":" + std::to_string(port) + "/connect/" + std::to_string(localID);
    PLOG_DEBUG << "Opening WebSocket to: " << url;
    ws->open(url);
    PLOG_DEBUG << "WebSocket open() called, waiting for connection...";
    wsFuture.get();
}

std::shared_ptr<rtc::PeerConnection> NetworkServer::createPeerConnection(const rtc::Configuration& config, std::weak_ptr<rtc::WebSocket> wws, PeerID id) {
    PLOG_DEBUG << "Creating peer connection for peer ID " << id;
    assert(!m_clients.contains(id));

    auto pc = std::make_shared<rtc::PeerConnection>(config);
    PLOG_DEBUG << "Created rtc::PeerConnection object";

    // Store the connection in our clients map
    {
        std::lock_guard lock(m_clientsMutex);
        auto clientConn = std::make_unique<ClientConnection>();
        clientConn->peerConnection = pc;
        m_clients[id] = std::move(clientConn);
        PLOG_DEBUG << "Stored connection in m_clients, total clients: " << m_clients.size();
    }

    pc->onStateChange([id](rtc::PeerConnection::State state) { PLOG_DEBUG << "Peer " << id << " state changed to: " << state; });
    pc->onGatheringStateChange(
        [id](rtc::PeerConnection::GatheringState state) { PLOG_DEBUG << "Peer " << id << " ICE gathering state: " << state; });
    pc->onLocalDescription([wws, id](rtc::Description description) {
        PLOG_DEBUG << "Peer " << id << " generated local " << description.typeString() << " (" << std::string(description).size() << " bytes)";
        nlohmann::json message = {{"id", id}, {"type", description.typeString()}, {"description", std::string(description)}};
        if (auto ws = wws.lock()) {
            PLOG_DEBUG << "Sending " << description.typeString() << " via websocket";
            ws->send(message.dump());
        } else {
            PLOG_ERROR << "[PeerConnection.onLocalDescription] WebSocket is closed, cannot send " << description.typeString();
        }
    });
    pc->onLocalCandidate([wws, id](rtc::Candidate candidate) {
        PLOG_DEBUG << "Peer " << id << " generated ICE candidate (mid=" << candidate.mid() << ", len=" << std::string(candidate).size() << ")";
        nlohmann::json message = {{"id", id}, {"type", "candidate"}, {"candidate", std::string(candidate)}, {"mid", candidate.mid()}};
        if (auto ws = wws.lock()) {
            ws->send(message.dump());
        } else {
            PLOG_ERROR << "[PeerConnection.onLocalCandidate] WebSocket is closed, cannot send candidate";
        }
    });

    pc->onDataChannel([this, id](std::shared_ptr<rtc::DataChannel> dc) {
        PLOG_DEBUG << "Peer " << id << " opened data channel with label: " << dc->label();

        if (dc->label() == TEST_DATACHANNEL) {
            PLOG_DEBUG << "Attaching test data channel handler for peer " << id;
            std::lock_guard lock(m_clientsMutex);
            if (auto it = m_clients.find(id); it != m_clients.end()) {
                it->second->m_testDataChannel = dc;
            }
        } else if (dc->label() == DEFAULT_DATACHANNEL) {
            PLOG_DEBUG << "Attaching state data channel handler for peer " << id;
            std::lock_guard lock(m_clientsMutex);
            if (auto it = m_clients.find(id); it != m_clients.end()) {
                it->second->m_stateDataChannel = dc;
            }
        } else {
            PLOG_DEBUG << "Unrecognized data channel label: " << dc->label();
        }
    });

    PLOG_DEBUG << "Creating local description (offer) for peer " << id;
    pc->createOffer();

    PLOG_DEBUG << "Successfully created peer connection for peer ID " << id;
    return pc;
}

void NetworkServer::run() {
    PLOG_DEBUG << "Network thread started";

    auto signallingUrlCStr = std::getenv("SIGNALING_URL");
    std::string signallingUrl = "localhost:9812";
    if (signallingUrlCStr) {
        signallingUrl = signallingUrlCStr;
    }
    PLOG_DEBUG << "SIGNALING_URL env=" << (signallingUrlCStr ? signallingUrl : "(not set, using default)") << ", using URL: " << signallingUrl;

    size_t colonPos = signallingUrl.find(':');
    std::string host = signallingUrl.substr(0, colonPos);
    uint16_t port = static_cast<uint16_t>(std::stoi(signallingUrl.substr(colonPos + 1)));
    PLOG_DEBUG << "Parsed signalling server: host=" << host << ", port=" << port;

    PLOG_DEBUG << "Starting signalling websocket connection";
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

        // WebRTC networking events are processed by libdatachannel's internal thread.

        const auto sleepTime = std::min(m_tickPeriod - remainingTime, Seconds(1.0f));
        if (sleepTime.count() > 0.0f) {
            std::this_thread::sleep_for(sleepTime);
        }
    }

    PLOG_DEBUG << "Network thread loop exited";
}

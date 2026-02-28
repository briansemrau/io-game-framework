#include "network_server.h"

#include <plog/Log.h>

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <mutex>
#include <nlohmann/json.hpp>

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
    if (m_networkThread.joinable()) {
        m_networkThread.join();
    }
    std::lock_guard lock(m_clientsMutex);
    m_clients.clear();
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
    rtc::Configuration config;

    // TODO put this in some config file
    const std::vector<std::string> iceServerURLs{
        {"stun:stun.l.google.com:19302"}, {"stun:stun.l.google.com:5349"},  {"stun:stun1.l.google.com:3478"}, {"stun:stun1.l.google.com:5349"},  {"stun:stun2.l.google.com:19302"},
        {"stun:stun2.l.google.com:5349"}, {"stun:stun3.l.google.com:3478"}, {"stun:stun3.l.google.com:5349"}, {"stun:stun4.l.google.com:19302"}, {"stun:stun4.l.google.com:5349"}};
    for (const auto& server : iceServerURLs) {
        config.iceServers.push_back(rtc::IceServer(server));
    }

    config.enableIceUdpMux = true;
    // m_localID = randomId(32);
    // std::string localID = generateRandomIDStr(16);
    PeerID localID = generateRandomPeerID();
    // PLOG_DEBUG << "Local ID: " << m_localID;

    auto ws = std::make_shared<rtc::WebSocket>();

    std::promise<void> wsPromise;
    auto wsFuture = wsPromise.get_future();

    ws->onOpen([&wsPromise]() {
        PLOG_DEBUG << "Websocket connected; ready to signal";
        wsPromise.set_value();
    });
    ws->onError([&wsPromise](std::string s) {
        PLOG_DEBUG << "Websocket error";
        wsPromise.set_exception(std::make_exception_ptr(std::runtime_error(s)));
    });
    ws->onClosed([]() { PLOG_DEBUG << "Websocket closed"; });

    ws->onMessage([this, &config, wws = static_cast<std::weak_ptr<rtc::WebSocket>>(ws)](auto data) {
        if (!std::holds_alternative<std::string>(data)) return;

        nlohmann::json message = nlohmann::json::parse(std::get<std::string>(data));

        auto it = message.find("id");
        if (it == message.end()) return;
        auto id_str = it->get<std::string>();
        PeerID id;
        try {
            id = std::stoull(id_str);
        } catch (const std::invalid_argument& ia) {
            PLOG_DEBUG << "Message peer ID is invalid: " << ia.what();
            return;
        }

        it = message.find("type");
        if (it == message.end()) return;
        auto type = it->get<std::string>();

        std::shared_ptr<rtc::PeerConnection> pc;
        if (auto jt = m_clients.find(id); jt != m_clients.end()) {
            pc = jt->second->peerConnection;
        } else if (type == "offer") {
            PLOG_DEBUG << "Answering to " << id;
            createPeerConnection(config, wws, id);
        } else {
            PLOG_DEBUG << "Peer connection already exists and type is " << type << ". How did we get here?";
            return;
        }

        if (type == "offer" || type == "answer") {
            auto sdp = message["description"].get<std::string>();
            pc->setRemoteDescription(rtc::Description(sdp, type));
        } else if (type == "candidate") {
            auto sdp = message["candidate"].get<std::string>();
            auto mid = message["mid"].get<std::string>();
            pc->addRemoteCandidate(rtc::Candidate(sdp, mid));
        }
    });

    const std::string url = (signalServerUrl.find("://") == std::string::npos ? "ws://" : "") + signalServerUrl + ":" + std::to_string(port) + "/" + std::to_string(localID);
    ws->open(url);
    PLOG_DEBUG << "Waiting for websocket to connect...";
    // wsPromise.get_future();
}

std::shared_ptr<rtc::PeerConnection> NetworkServer::createPeerConnection(const rtc::Configuration& config, std::weak_ptr<rtc::WebSocket> wws, PeerID id) {
    assert(!m_clients.contains(id));

    auto pc = std::make_shared<rtc::PeerConnection>(config);

    pc->onStateChange([](rtc::PeerConnection::State state) { PLOG_DEBUG << "State: " << state; });
    pc->onGatheringStateChange([](rtc::PeerConnection::GatheringState state) { PLOG_DEBUG << "Gathering state: " << state; });
    pc->onLocalDescription([wws, id](rtc::Description description) {
        nlohmann::json message = {{"id", id}, {"type", description.typeString()}, {"description", std::string(description)}};
        if (auto ws = wws.lock()) ws->send(message.dump());
    });
    pc->onLocalCandidate([wws, id](rtc::Candidate candidate) {
        nlohmann::json message = {{"id", id}, {"type", "candidate"}, {"candidate", std::string(candidate)}, {"mid", candidate.mid()}};
        if (auto ws = wws.lock()) ws->send(message.dump());
    });

    pc->onDataChannel([this, id](std::shared_ptr<rtc::DataChannel> dc) {
        // Only the server sets up datachannels
        PLOG_DEBUG << "Unexpected DataChannel from " << id << " received with label \"" << dc->label() << "\"";
    });

    return pc;
}

void NetworkServer::run() {
    const char* signallingUrlEnv = std::getenv("SIGNALING_URL");
    std::string signallingUrl = signallingUrlEnv ? signallingUrlEnv : "localhost:9812";

    size_t colonPos = signallingUrl.find(':');
    std::string host = signallingUrl.substr(0, colonPos);
    uint16_t port = static_cast<uint16_t>(std::stoi(signallingUrl.substr(colonPos + 1)));

    PLOG_DEBUG << "Connecting to signalling server at " << host << ":" << port;
    startSignallingWebsocket(host, port);

    auto previousTime = std::chrono::steady_clock::now();
    auto remainingTime = Seconds::zero();
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
        // Callbacks (onMessage, onDataChannel, etc.) are invoked from that thread and
        // are thread-safe due to the mutex protection in each callback handler.

        const auto sleepTime = std::min(m_tickPeriod - remainingTime, Seconds(1.0f));
        if (sleepTime.count() > 0.0f) {
            std::this_thread::sleep_for(sleepTime);
        }
    }
}

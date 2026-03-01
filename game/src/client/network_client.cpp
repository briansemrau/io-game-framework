#include "network_client.h"

#include <assert.h>
#include <plog/Log.h>

#include <chrono>
#include <cstring>
#include <future>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>

#include "network_common.h"

NetworkClient::NetworkClient(const Game& game) : m_game(game) { PLOG_DEBUG << "Constructor called"; }

NetworkClient::~NetworkClient() {
    PLOG_DEBUG << "Destructor called";
    disconnect();
}

void NetworkClient::connect(const std::string& signalServerUrl, uint16_t port, PeerID serverID) {
    PLOG_DEBUG << "Connecting to signalling server " << signalServerUrl << ":" << port << " to reach server " << serverID;

    rtc::Configuration config;
    PLOG_DEBUG << "Created rtc::Configuration";

    // TODO put this in some config file
    const std::vector<std::string> iceServerURLs{
        {"stun:stun.l.google.com:19302"}, {"stun:stun.l.google.com:5349"},  {"stun:stun1.l.google.com:3478"}, {"stun:stun1.l.google.com:5349"},  {"stun:stun2.l.google.com:19302"},
        {"stun:stun2.l.google.com:5349"}, {"stun:stun3.l.google.com:3478"}, {"stun:stun3.l.google.com:5349"}, {"stun:stun4.l.google.com:19302"}, {"stun:stun4.l.google.com:5349"}};
    PLOG_DEBUG << "Adding " << iceServerURLs.size() << " ICE servers";
    for (const auto& server : iceServerURLs) {
        config.iceServers.push_back(rtc::IceServer(server));
    }

    // config.enableIceUdpMux = true; // TODO enable on server
    m_localID = generateRandomPeerID();
    PLOG_DEBUG << "Generated local PeerID: " << m_localID;

    auto ws = std::make_shared<rtc::WebSocket>();

    std::promise<void> wsPromise;
    auto wsFuture = wsPromise.get_future();

    ws->onOpen([this, &wsPromise, localID = m_localID, ws, serverID, config]() mutable {
        PLOG_DEBUG << "WebSocket connected for client (localID=" << localID << ")";

        nlohmann::json registerMsg = {{"id", std::to_string(localID)}, {"type", "register"}, {"role", "client"}};
        ws->send(registerMsg.dump());
        PLOG_DEBUG << "Sent registration message for client " << localID;

        wsPromise.set_value();
    });
    ws->onError([&wsPromise, localID = m_localID](std::string s) {
        PLOG_ERROR << "WebSocket error for client (localID=" << localID << "): " << s;
        wsPromise.set_exception(std::make_exception_ptr(std::runtime_error(s)));
    });
    ws->onClosed([localID = m_localID]() { PLOG_DEBUG << "WebSocket closed for client (localID=" << localID << ")"; });

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
            PLOG_ERROR << "Failed to parse JSON: " << e.what();
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
            PLOG_ERROR << "Message peer ID is invalid: " << ia.what() << " (value: " << id_str << ")";
            return;
        }

        it = message.find("type");
        if (it == message.end()) {
            PLOG_DEBUG << "Message missing 'type' field";
            return;
        }
        auto type = it->get<std::string>();
        PLOG_DEBUG << "Message 'type' field: " << type;

        if (m_peerConnection != nullptr) {
            PLOG_DEBUG << "Peer connection already exists, ignoring message type=" << type;
        } else if (!m_peerConnection && type == "offer") {
            // TODO: P2P capability - enable when ready to support client-to-client connections
            // PLOG_DEBUG << "Received 'offer' from peer " << id << ", creating peer connection";
            // createPeerConnection(config, wws, id);
            PLOG_DEBUG << "Received 'offer' from peer " << id << ", ignoring (P2P disabled)";
        } else {
            PLOG_ERROR << "No peer connection exists and type is " << type;
            return;
        }

        if (type == "offer" || type == "answer") {
            PLOG_DEBUG << "Processing SDP " << type << " from peer " << id;
            auto sdp = message["description"].get<std::string>();
            PLOG_DEBUG << "SDP " << type << " length: " << sdp.size() << " bytes";
            m_peerConnection->setRemoteDescription(rtc::Description(sdp, type));
            PLOG_DEBUG << "Set remote " << type << " successfully";
        } else if (type == "candidate") {
            PLOG_DEBUG << "Processing ICE candidate from peer " << id;
            auto sdp = message["candidate"].get<std::string>();
            auto mid = message["mid"].get<std::string>();
            PLOG_DEBUG << "ICE candidate mid=" << mid << " sdp_length=" << sdp.size();
            m_peerConnection->addRemoteCandidate(rtc::Candidate(sdp, mid));
            PLOG_DEBUG << "Added remote candidate successfully";
        }
    });

    const std::string url =
        (signalServerUrl.find("://") == std::string::npos ? "ws://" : "") + signalServerUrl + ":" + std::to_string(port) + "/connect/" + std::to_string(m_localID);
    PLOG_DEBUG << "Opening WebSocket to: " << url;
    ws->open(url);
    PLOG_DEBUG << "WebSocket open() called, waiting for connection...";
    wsFuture.get();
    // Initiate connection to server by creating an offer
    startServerConnection(ws, serverID, std::move(config));
}

// TODO: P2P capability - uncomment when ready to support client-to-client connections
/*
void NetworkClient::createPeerConnection(const rtc::Configuration& config, std::weak_ptr<rtc::WebSocket> wws, PeerID id) {
    PLOG_DEBUG << "Creating peer connection for peer ID " << id;
    assert(m_peerConnection == nullptr);

    m_peerConnection = std::make_shared<rtc::PeerConnection>(config);
    PLOG_DEBUG << "Created rtc::PeerConnection object";

    m_peerConnection->onStateChange([id](rtc::PeerConnection::State state) { PLOG_DEBUG << "Peer " << id << " state changed to: " << state; });
    m_peerConnection->onGatheringStateChange([id](rtc::PeerConnection::GatheringState state) { PLOG_DEBUG << "Peer " << id << " ICE gathering state: " << state; });
    m_peerConnection->onLocalDescription([wws, id](rtc::Description description) {
        PLOG_DEBUG << "Peer " << id << " generated local " << description.typeString() << " (" << std::string(description).size() << " bytes)";
        nlohmann::json message = {{"id", id}, {"type", description.typeString()}, {"description", std::string(description)}};
        if (auto ws = wws.lock()) {
            PLOG_DEBUG << "Sending " << description.typeString() << " via websocket";
            ws->send(message.dump());
        } else {
            PLOG_ERROR << "WebSocket is closed, cannot send " << description.typeString();
        }
    });
    m_peerConnection->onLocalCandidate([wws, id](rtc::Candidate candidate) {
        PLOG_DEBUG << "Peer " << id << " generated ICE candidate (mid=" << candidate.mid() << ", len=" << std::string(candidate).size() << ")";
        nlohmann::json message = {{"id", id}, {"type", "candidate"}, {"candidate", std::string(candidate)}, {"mid", candidate.mid()}};
        if (auto ws = wws.lock()) {
            ws->send(message.dump());
        } else {
            PLOG_ERROR << "WebSocket is closed, cannot send candidate";
        }
    });

    m_peerConnection->onDataChannel([this, id](std::shared_ptr<rtc::DataChannel> dc) {
        PLOG_DEBUG << "Peer " << id << " opened data channel with label: " << dc->label();

        if (dc->label() == TEST_DATACHANNEL) {
            PLOG_DEBUG << "Attaching test data channel handler for peer " << id;
            m_testDataChannel = dc;
            m_testDataChannel->onOpen([this, id, wdc = static_cast<std::weak_ptr<rtc::DataChannel>>(dc)]() {
                PLOG_DEBUG << "Test datachannel opening for peer " << id;
                if (auto dc = wdc.lock()) {
                    PLOG_DEBUG << "Datachannel \"" << dc->label() << "\" (from id: " << id << ") has opened";
                    dc->send("Test hello from " + std::to_string(m_localID));
                    PLOG_DEBUG << "Sent test hello message";
                }
            });
            m_testDataChannel->onClosed([id, wdc = static_cast<std::weak_ptr<rtc::DataChannel>>(dc)]() {
                if (auto dc = wdc.lock()) PLOG_DEBUG << "Datachannel \"" << dc->label() << "\" (from id: " << id << ") has closed";
            });
            m_testDataChannel->onMessage([id](auto data) {
                if (std::holds_alternative<std::string>(data)) {
                    PLOG_INFO << "[DataChannel.onMessage] Message from " << id << ": " << std::get<std::string>(data);
                } else {
                    PLOG_DEBUG << "Binary message from " << id;
                }
            });
        } else if (dc->label() == DEFAULT_DATACHANNEL) {
            PLOG_DEBUG << "Attaching state data channel handler for peer " << id;
            m_stateDataChannel = dc;
            m_stateDataChannel->onOpen([wdc = static_cast<std::weak_ptr<rtc::DataChannel>>(dc)]() {
                if (auto dc = wdc.lock()) PLOG_DEBUG << "Datachannel \"" << dc->label() << "\" has opened";
            });
            m_stateDataChannel->onClosed([id, wdc = static_cast<std::weak_ptr<rtc::DataChannel>>(dc)]() {
                if (auto dc = wdc.lock()) PLOG_DEBUG << "Datachannel \"" << dc->label() << "\" (from id: " << id << ") has closed";
            });
            m_stateDataChannel->onMessage([this](auto data) {
                PLOG_DEBUG << "Received state message (" << std::get<rtc::binary>(data).size() << " bytes)";
                onStateMessage(std::get<rtc::binary>(data));
            });
        } else {
            PLOG_DEBUG << "Unrecognized datachannel label: \"" << dc->label() << "\"";
        }
    });

    PLOG_DEBUG << "Creating local description (answer) for peer " << id;
    m_peerConnection->createAnswer();
    PLOG_DEBUG << "Successfully created peer connection for peer ID " << id;
}
*/

void NetworkClient::onStateMessage(std::vector<std::byte> data) {
    // TODO
}

void NetworkClient::disconnect() {
    PLOG_DEBUG << "Called";
    if (m_peerConnection) {
        PLOG_DEBUG << "Closing peer connection";
        m_peerConnection->close();
        m_peerConnection.reset();
        PLOG_DEBUG << "Peer connection closed and reset";
    } else {
        PLOG_DEBUG << "No peer connection to close";
    }
    m_testDataChannel.reset();
    m_stateDataChannel.reset();
    PLOG_DEBUG << "Completed";
}

bool NetworkClient::isConnected() const {
    bool connected = m_peerConnection != nullptr && m_peerConnection->state() == rtc::PeerConnection::State::Connected;
    PLOG_DEBUG << "Check result: " << (connected ? "true" : "false");
    return connected;
}

void NetworkClient::startServerConnection(std::shared_ptr<rtc::WebSocket> ws, PeerID serverID, rtc::Configuration config) {
    PLOG_DEBUG << "Initiating connection from client " << m_localID << " to server " << serverID;

    auto pc = std::make_shared<rtc::PeerConnection>(config);

    pc->onStateChange([serverID](rtc::PeerConnection::State state) { PLOG_DEBUG << "Peer " << serverID << " state changed to: " << state; });

    pc->onGatheringStateChange([serverID](rtc::PeerConnection::GatheringState state) { PLOG_DEBUG << "Peer " << serverID << " ICE gathering state: " << state; });

    pc->onLocalDescription([ws, serverID](rtc::Description description) {
        PLOG_DEBUG << "Generated local " << description.typeString() << " for server " << serverID;
        nlohmann::json message = {{"id", std::to_string(serverID)}, {"type", description.typeString()}, {"description", std::string(description)}};
        if (ws) {
            PLOG_DEBUG << "Sending " << description.typeString() << " to server " << serverID;
            ws->send(message.dump());
        } else {
            PLOG_ERROR << "WebSocket is closed, cannot send " << description.typeString();
        }
    });

    pc->onLocalCandidate([ws, serverID](rtc::Candidate candidate) {
        PLOG_DEBUG << "Generated ICE candidate for server " << serverID;
        nlohmann::json message = {{"id", std::to_string(serverID)}, {"type", "candidate"}, {"candidate", std::string(candidate)}, {"mid", candidate.mid()}};
        if (ws) {
            ws->send(message.dump());
        } else {
            PLOG_ERROR << "WebSocket is closed, cannot send candidate";
        }
    });

    pc->onDataChannel([this, serverID](std::shared_ptr<rtc::DataChannel> dc) {
        PLOG_DEBUG << "Server " << serverID << " opened data channel with label: " << dc->label();

        if (dc->label() == TEST_DATACHANNEL) {
            PLOG_DEBUG << "Attaching test data channel handler for server " << serverID;
            m_testDataChannel = dc;
        } else if (dc->label() == STATE_DATACHANNEL) {
            PLOG_DEBUG << "Attaching state data channel handler for server " << serverID;
            m_stateDataChannel = dc;
        } else {
            PLOG_DEBUG << "Unrecognized data channel label: " << dc->label();
        }
    });

    m_peerConnection = pc;
    PLOG_DEBUG << "Creating offer for server " << serverID;
    pc->createOffer();
    PLOG_DEBUG << "Successfully initiated connection to server " << serverID;
}

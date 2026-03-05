#include "network_client.h"

#include <plog/Log.h>

#include <cassert>
#include <chrono>
#include <future>
#include <memory>
#include <nlohmann/json.hpp>

#include "network_common.h"

NetworkClient::NetworkClient(const Game &game) : m_game(game) {}

NetworkClient::~NetworkClient() { disconnect(); }

void NetworkClient::connect(const std::string &signalServerUrl, uint16_t port, PeerID serverID) {
    PLOG_INFO << "Connecting to signalling server " << signalServerUrl << ":" << port << " to reach server " << serverID;
    startSignallingWebsocket(signalServerUrl, port);
    // createServerConnection(serverID); // client does not offer
    if (m_signallingWebsocket) {
        nlohmann::json joinMsg = { { "id", std::to_string(serverID) }, { "type", "join" }, { "role", "client" } };
        m_signallingWebsocket->send(joinMsg.dump());
    }
}

void NetworkClient::startSignallingWebsocket(const std::string &signalServerUrl, const uint16_t port) {
    m_localID = generateRandomPeerID();
    PLOG_INFO << "Generated local PeerID: " << m_localID;

    m_signallingWebsocket = std::make_shared<rtc::WebSocket>();
    auto &ws = m_signallingWebsocket;
    std::promise<void> wsPromise;
    auto wsFuture = wsPromise.get_future();

    ws->onOpen([&wsPromise, localID = m_localID, ws]() {
        PLOG_INFO << "WebSocket connected for client (localID=" << localID << ")";
        nlohmann::json registerMsg = { { "id", std::to_string(localID) }, { "type", "register" }, { "role", "client" } };
        ws->send(registerMsg.dump());
        wsPromise.set_value();
    });

    ws->onError([localID = m_localID, &wsPromise](const std::string &error) {
        PLOG_ERROR << "WebSocket error for client (localID=" << localID << "): " << error;
        wsPromise.set_exception(std::make_exception_ptr(std::runtime_error(error)));
    });

    ws->onClosed([localID = m_localID]() { PLOG_INFO << "WebSocket closed for client (localID=" << localID << ")"; });

    ws->onMessage([this, wws = std::weak_ptr<rtc::WebSocket>(ws)](auto data) {
        if (!std::holds_alternative<std::string>(data)) {
            return;
        }

        const std::string rawData = std::get<std::string>(data);

        nlohmann::json message;
        try {
            message = nlohmann::json::parse(rawData);
        } catch (const std::exception &e) {
            PLOG_ERROR << "Failed to parse JSON: " << e.what();
            return;
        }

        handleSignallingMessage(message);
    });

    const std::string url =
        (signalServerUrl.find("://") == std::string::npos ? "ws://" : "") + signalServerUrl + ":" + std::to_string(port) + "/connect/" + std::to_string(m_localID);
    PLOG_DEBUG << "Opening WebSocket to: " << url;
    ws->open(url);
    PLOG_DEBUG << "WebSocket open() called, waiting for connection...";
    if (wsFuture.wait_for(std::chrono::seconds(10)) == std::future_status::timeout) {
        PLOG_ERROR << "WebSocket connection timeout after 10 seconds";
        return;
    }
}

void NetworkClient::handleSignallingMessage(const nlohmann::json &message) {
    auto idIt = message.find("id");
    if (idIt == message.end()) {
        PLOG_ERROR << "Message missing 'id' field";
        return;
    }

    PeerID id{};
    try {
        id = std::stoull(idIt->get<std::string>());
    } catch (const std::invalid_argument &) {
        PLOG_ERROR << "Invalid peer ID in message";
        return;
    }

    auto typeIt = message.find("type");
    if (typeIt == message.end()) {
        PLOG_ERROR << "Message missing 'type' field";
        return;
    }
    const std::string type = typeIt->get<std::string>();
    if (type == "join") {
        PLOG_ERROR << "Unexpected join message received by client";
        return;
    }

    std::shared_ptr<rtc::PeerConnection> pc;
    if (m_connection.peerConnection) {
        pc = m_connection.peerConnection;
    } else if (type == "offer") {
        createServerConnection(id);
        pc = m_connection.peerConnection;
    } else {
        PLOG_ERROR << "No connection exists for peer " << id << " and type is " << type;
        return;
    }

    if (type == "offer" || type == "answer") {
        auto sdp = message["description"].get<std::string>();
        m_connection.peerConnection->setRemoteDescription(rtc::Description(sdp, type));
    } else if (type == "candidate") {
        auto candidate = message["candidate"].get<std::string>();
        auto mid = message["mid"].get<std::string>();
        m_connection.peerConnection->addRemoteCandidate(rtc::Candidate(candidate, mid));
    }
}

void NetworkClient::createServerConnection(const PeerID id) {
    PLOG_INFO << "Initiating connection from client " << m_localID << " to server " << id;

    rtc::Configuration config;
    for (const auto &url : getDefaultIceServerUrls()) {
        config.iceServers.emplace_back(url);
    }

    m_connection.peerConnection = std::make_shared<rtc::PeerConnection>(config);
    auto &pc = m_connection.peerConnection;

    pc->onStateChange([id](rtc::PeerConnection::State state) { PLOG_DEBUG << "Peer " << id << " state: " << state; });

    pc->onGatheringStateChange([id](rtc::PeerConnection::GatheringState state) { PLOG_DEBUG << "Peer " << id << " ICE gathering state: " << state; });

    pc->onLocalDescription([ws = m_signallingWebsocket, id](const rtc::Description &description) {
        nlohmann::json message = { { "id", std::to_string(id) }, { "type", description.typeString() }, { "description", std::string(description) } };
        if (ws) {
            PLOG_DEBUG << "Sending " << description.typeString() << " via websocket";
            ws->send(message.dump());
        } else {
            PLOG_ERROR << "WebSocket is closed, cannot send " << description.typeString();
        }
    });

    pc->onLocalCandidate([ws = m_signallingWebsocket, id](const rtc::Candidate &candidate) {
        nlohmann::json message = { { "id", std::to_string(id) }, { "type", "candidate" }, { "candidate", std::string(candidate) }, { "mid", candidate.mid() } };
        if (ws) {
            ws->send(message.dump());
        } else {
            PLOG_ERROR << "WebSocket is closed, cannot send candidate";
        }
    });

    // Inbound data channels
    pc->onDataChannel([this, id](const std::shared_ptr<rtc::DataChannel> &dc) {
        PLOG_INFO << "Server " << id << " opened data channel: " << dc->label();
        if (dc->label() == STATE_DATACHANNEL) {
            m_connection.stateDataChannel = dc;
            dc->onOpen([id, wdc = std::weak_ptr<rtc::DataChannel>(dc)]() { PLOG_INFO << "State datachannel opened for server " << id; });
            dc->onClosed([id]() { PLOG_INFO << "State datachannel closed for server " << id; });
            dc->onMessage([this, id, wdc = std::weak_ptr<rtc::DataChannel>(dc)](auto data) {
                if (std::holds_alternative<std::string>(data)) {
                    auto &strData = std::get<std::string>(data);
                    PLOG_DEBUG << "Received state message from server " << id << ": " << strData;
                } else {
                    auto &binaryData = std::get<rtc::binary>(data);
                    PLOG_DEBUG << "Received state message from server " << id << " (" << binaryData.size() << " bytes)";
                    onStateMessage(binaryData);
                }
            });
        } else if (dc->label() == TEST_DATACHANNEL) {
            m_connection.testDataChannel = dc;
            dc->onOpen([id, wdc = std::weak_ptr<rtc::DataChannel>(dc)]() { PLOG_INFO << "Test datachannel opened for server " << id; });
            dc->onClosed([id]() { PLOG_INFO << "Test datachannel closed for server " << id; });
            dc->onMessage([this, id, wdc = std::weak_ptr<rtc::DataChannel>(dc)](auto data) {
                if (std::holds_alternative<std::string>(data)) {
                    auto &strData = std::get<std::string>(data);
                    PLOG_DEBUG << "Received test message from server " << id << ": " << strData;
                } else {
                    auto &binaryData = std::get<rtc::binary>(data);
                    PLOG_DEBUG << "Received test message from server " << id << " (" << binaryData.size() << " bytes)";
                }
            });
        } else {
            PLOG_DEBUG << "Unexpected data channel label: " << dc->label();
        }
    });
}

void NetworkClient::onStateMessage(const std::vector<std::byte> &data) { PLOG_DEBUG << "Received state message (" << data.size() << " bytes)"; }

void NetworkClient::disconnect() {
    if (m_connection.peerConnection) {
        m_connection.peerConnection->close();
        m_connection.peerConnection.reset();
    }
    m_connection.testDataChannel.reset();
    m_connection.stateDataChannel.reset();
}

bool NetworkClient::isConnected() const { return m_connection.peerConnection != nullptr && m_connection.peerConnection->state() == rtc::PeerConnection::State::Connected; }
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

NetworkClient::NetworkClient(const Game& game) : m_game(game) {}

NetworkClient::~NetworkClient() { disconnect(); }

void NetworkClient::connect(const std::string& serverUrl, uint16_t serverPort) {
    rtc::Configuration config;

    // TODO put this in some config file
    const std::vector<std::string> iceServerURLs{
        {"stun:stun.l.google.com:19302"}, {"stun:stun.l.google.com:5349"},  {"stun:stun1.l.google.com:3478"}, {"stun:stun1.l.google.com:5349"},  {"stun:stun2.l.google.com:19302"},
        {"stun:stun2.l.google.com:5349"}, {"stun:stun3.l.google.com:3478"}, {"stun:stun3.l.google.com:5349"}, {"stun:stun4.l.google.com:19302"}, {"stun:stun4.l.google.com:5349"}};
    for (const auto& server : iceServerURLs) {
        config.iceServers.push_back(rtc::IceServer(server));
    }

    // config.enableIceUdpMux = true; // TODO enable on server
    m_localID = generateRandomIDStr(32);
    PLOG_DEBUG << "Local ID: " << m_localID;

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
        auto id = it->get<std::string>();

        it = message.find("type");
        if (it == message.end()) return;
        auto type = it->get<std::string>();

        if (m_peerConnection != nullptr) {
            // do nothing
        } else if (!m_peerConnection && type == "offer") {
            PLOG_DEBUG << "Answering to " << id;
            createPeerConnection(config, wws, id);
        } else {
            PLOG_DEBUG << "Peer connection already exists and type is " << type << ". How did we get here?";
            return;
        }

        if (type == "offer" || type == "answer") {
            auto sdp = message["description"].get<std::string>();
            m_peerConnection->setRemoteDescription(rtc::Description(sdp, type));
        } else if (type == "candidate") {
            auto sdp = message["candidate"].get<std::string>();
            auto mid = message["mid"].get<std::string>();
            m_peerConnection->addRemoteCandidate(rtc::Candidate(sdp, mid));
        }
    });

    const std::string url = (serverUrl.find("://") == std::string::npos ? "ws://" : "") + serverUrl + ":" + std::to_string(serverPort) + "/" + m_localID;
    ws->open(url);
    PLOG_DEBUG << "Waiting for websocket to connect...";
}

void NetworkClient::createPeerConnection(const rtc::Configuration& config, std::weak_ptr<rtc::WebSocket> wws, std::string id) {
    assert(m_peerConnection == nullptr);

    m_peerConnection = std::make_shared<rtc::PeerConnection>(config);

    m_peerConnection->onStateChange([](rtc::PeerConnection::State state) { PLOG_DEBUG << "State: " << state; });
    m_peerConnection->onGatheringStateChange([](rtc::PeerConnection::GatheringState state) { PLOG_DEBUG << "Gathering state: " << state; });
    m_peerConnection->onLocalDescription([wws, id](rtc::Description description) {
        nlohmann::json message = {{"id", id}, {"type", description.typeString()}, {"description", std::string(description)}};
        if (auto ws = wws.lock()) ws->send(message.dump());
    });
    m_peerConnection->onLocalCandidate([wws, id](rtc::Candidate candidate) {
        nlohmann::json message = {{"id", id}, {"type", "candidate"}, {"candidate", std::string(candidate)}, {"mid", candidate.mid()}};
        if (auto ws = wws.lock()) ws->send(message.dump());
    });

    m_peerConnection->onDataChannel([this, id](std::shared_ptr<rtc::DataChannel> dc) {
        PLOG_DEBUG << "DataChannel from " << id << " received with label \"" << dc->label() << "\"";

        if (dc->label() == TEST_DATACHANNEL) {
            m_testDataChannel = dc;
            m_testDataChannel->onOpen([this, id, wdc = static_cast<std::weak_ptr<rtc::DataChannel>>(dc)]() {
                if (auto dc = wdc.lock()) {
                    PLOG_DEBUG << "Datachannel \"" << dc->label() << "\" (from id: " << id << ") has opened";
                    dc->send("Test hello from " + m_localID);
                }
            });
            m_testDataChannel->onClosed([id, wdc = static_cast<std::weak_ptr<rtc::DataChannel>>(dc)]() {
                if (auto dc = wdc.lock()) PLOG_DEBUG << "Datachannel \"" << dc->label() << "\" (from id: " << id << ") has closed";
            });
            m_testDataChannel->onMessage([id](auto data) {
                if (std::holds_alternative<std::string>(data)) {
                    PLOG_INFO << "Message from " << id << ": " << std::get<std::string>(data);
                }
            });
        } else if (dc->label() == DEFAULT_DATACHANNEL) {
            m_stateDataChannel = dc;
            m_stateDataChannel->onOpen([wdc = static_cast<std::weak_ptr<rtc::DataChannel>>(dc)]() {
                if (auto dc = wdc.lock()) PLOG_DEBUG << "Datachannel \"" << dc->label() << "\" has opened";
            });
            m_stateDataChannel->onClosed([id, wdc = static_cast<std::weak_ptr<rtc::DataChannel>>(dc)]() {
                if (auto dc = wdc.lock()) PLOG_DEBUG << "Datachannel \"" << dc->label() << "\" (from id: " << id << ") has closed";
            });
            m_stateDataChannel->onMessage([this](auto data) { onStateMessage(std::get<rtc::binary>(data)); });
        } else {
            PLOG_DEBUG << "Unrecognized datachannel label: \"" << dc->label() << "\"";
        }
    });
}

void NetworkClient::onStateMessage(std::vector<std::byte> data) {
    // TODO
}

void NetworkClient::disconnect() {
    if (m_peerConnection) {
        m_peerConnection->close();
        m_peerConnection.reset();
    }
    m_testDataChannel.reset();
    m_stateDataChannel.reset();
}

bool NetworkClient::isConnected() const {
    return m_peerConnection != nullptr && m_peerConnection->state() == rtc::PeerConnection::State::Connected;
}

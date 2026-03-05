#ifndef NETWORK_CLIENT_H
#define NETWORK_CLIENT_H

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <vector>

#include "game.h"
#include "network_common.h"
#include "rtc/rtc.hpp"

class NetworkClient {
public:
    NetworkClient(const Game &);
    ~NetworkClient();

    NetworkClient(const NetworkClient &) = delete;
    NetworkClient &operator=(const NetworkClient &) = delete;
    NetworkClient(NetworkClient &&) = delete;
    NetworkClient &operator=(NetworkClient &&) = delete;

    void connect(const std::string &signalServerUrl, uint16_t port, PeerID);
    void disconnect();
    bool isConnected() const;

private:
    using Seconds = std::chrono::duration<float, std::ratio<1>>;

    void startSignallingWebsocket(const std::string &signalServerUrl, uint16_t port);
    void handleSignallingMessage(const nlohmann::json &);

    void createServerConnection(PeerID);
    // TODO: P2P capability? shall we support client-to-client connections?
    // void createPeerConnection(const rtc::Configuration&, std::weak_ptr<rtc::WebSocket>, PeerID id);

    void onStateMessage(const std::vector<std::byte> &);

    const Game &m_game;

    PeerID m_localID{};
    std::shared_ptr<rtc::WebSocket> m_signallingWebsocket;

    NetworkConnection m_connection;
};

#endif  // NETWORK_CLIENT_H

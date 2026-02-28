#ifndef NETWORK_CLIENT_H
#define NETWORK_CLIENT_H

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <rtc/rtc.hpp>
#include <thread>
#include <variant>
#include <vector>

#include "game.h"
#include "network_common.h"

class NetworkClient {
public:
    using Seconds = std::chrono::duration<float, std::ratio<1>>;

    NetworkClient(const Game&);
    ~NetworkClient();

    NetworkClient(const NetworkClient&) = delete;
    NetworkClient& operator=(const NetworkClient&) = delete;
    NetworkClient(NetworkClient&&) = delete;
    NetworkClient& operator=(NetworkClient&&) = delete;

    void connect(const std::string& signalServerUrl, uint16_t port);
    void disconnect();
    bool isConnected() const;

private:
    void createPeerConnection(const rtc::Configuration &, std::weak_ptr<rtc::WebSocket>, PeerID id);

    void onStateMessage(std::vector<std::byte>);

    const Game &m_game;

    PeerID m_localID;
    std::shared_ptr<rtc::PeerConnection> m_peerConnection;
    std::shared_ptr<rtc::DataChannel> m_testDataChannel;
    std::shared_ptr<rtc::DataChannel> m_stateDataChannel;
};

#endif  // NETWORK_CLIENT_H

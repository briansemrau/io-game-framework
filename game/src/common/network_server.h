#ifndef NETWORK_SERVER_H
#define NETWORK_SERVER_H

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <rtc/rtc.hpp>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "game.h"
#include "network_common.h"
#include "nlohmann/json_fwd.hpp"

class NetworkServer {
public:
    NetworkServer(const Game&);
    ~NetworkServer();

    NetworkServer(const NetworkServer&) = delete;
    NetworkServer& operator=(const NetworkServer&) = delete;
    NetworkServer(NetworkServer&&) = delete;
    NetworkServer& operator=(NetworkServer&&) = delete;

    void start();
    void stop();
    bool isRunning() const;

private:
    using Seconds = std::chrono::duration<float, std::ratio<1>>;

    void startSignallingWebsocket(const std::string& signalServerUrl, const uint16_t port);
    void handleSignallingMessage(const nlohmann::json& message);

    void createClientConnection(const PeerID);

    void run();

    const Game& m_game;

    std::atomic<bool> m_running{false};
    std::thread m_networkThread;
    Seconds m_tickPeriod{0.1f};

    PeerID m_localID;
    std::shared_ptr<rtc::WebSocket> m_signallingWebsocket;

    std::mutex m_clientsMutex;
    std::unordered_map<PeerID, NetworkConnection> m_clients;
    // std::atomic<PeerID> m_nextClientId{1};
};
#endif  // NETWORK_SERVER_H

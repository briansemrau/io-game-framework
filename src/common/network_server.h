#ifndef NETWORK_SERVER_H
#define NETWORK_SERVER_H

// =============================================================================
// NetworkServer - Threaded WebRTC DataChannel Server
// =============================================================================
//
// WEBRTC CONCEPTS (for those familiar with WebSockets/TCP/UDP):
//
// Unlike WebSockets where you simply connect to a URL, WebRTC requires a
// "signaling" phase where peers exchange connection information out-of-band.
// This class handles the WebRTC side, but you'll need a separate mechanism
// (HTTP endpoint, WebSocket server, etc.) to exchange SDP and ICE candidates.
//
// FLOW:
// 1. Client requests to connect → server creates PeerConnection
// 2. Server generates SDP offer (localDescription) → send to client via signaling
// 3. Client generates SDP answer → send to server via setRemoteDescription()
// 4. Both sides exchange ICE candidates (network paths) via setRemoteCandidate()
// 5. DataChannel opens, binary messages flow bidirectionally
//
// ICE SERVERS:
// - STUN servers help discover your public IP (e.g., "stun:stun.l.google.com:19302")
// - TURN servers act as relay if direct connection fails (for NAT traversal)
//
// THREADING:
// - All WebRTC callbacks run on the network thread
// - send()/broadcast() are thread-safe (queue messages for network thread)
// - Callbacks (onMessage, onConnect, onDisconnect) fire from network thread
//   → dispatch to your game thread as needed
// =============================================================================

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

class NetworkServer {
public:
    using Seconds = std::chrono::duration<float, std::ratio<1>>;

    NetworkServer(Game& game);
    ~NetworkServer();

    NetworkServer(const NetworkServer&) = delete;
    NetworkServer& operator=(const NetworkServer&) = delete;
    NetworkServer(NetworkServer&&) = delete;
    NetworkServer& operator=(NetworkServer&&) = delete;

    void start();
    void stop();
    bool isRunning() const;

private:
    // TODO move to network_common; reuse in network_client
    struct ClientConnection {
        std::string localDescription;
        std::vector<std::string> pendingCandidates;
        std::shared_ptr<rtc::PeerConnection> peerConnection;
        std::shared_ptr<rtc::DataChannel> m_testDataChannel;
        std::shared_ptr<rtc::DataChannel> m_stateDataChannel;
    };

    void startSignallingWebsocket(const std::string& signalServerUrl, uint16_t port);

    std::shared_ptr<rtc::PeerConnection> createPeerConnection(const rtc::Configuration &, std::weak_ptr<rtc::WebSocket>, PeerID id);

    void run();

    const Game &m_game;

    std::atomic<bool> m_running{false};
    std::thread m_networkThread;
    Seconds m_tickPeriod{0.1f};

    std::mutex m_clientsMutex;
    std::unordered_map<PeerID, std::unique_ptr<ClientConnection>> m_clients;
    // std::atomic<PeerID> m_nextClientId{1};

    // Tx message queue
    struct OutgoingMessage {
        PeerID clientId;
        std::vector<std::byte> data;
        bool broadcast;
    };
    std::mutex m_outgoingMutex;
    std::condition_variable m_outgoingCv;
    std::queue<OutgoingMessage> m_outgoingQueue;
};
#endif  // NETWORK_SERVER_H

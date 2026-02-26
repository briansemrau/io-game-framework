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
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <rtc/rtc.hpp>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include <cstddef>

#include "game.h"

class NetworkServer {
public:
    using ClientId = uint32_t;
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

    // Send data to a specific client or all connected clients.
    // Thread-safe: queues message for the network thread to process.
    void send(ClientId clientId, const std::vector<std::byte>& data);
    void broadcast(const std::vector<std::byte>& data);

    // Accept an incoming WebRTC connection request.
    // Returns the new client ID that can be used to track this connection.
    // After calling this, use getLocalDescription() to get the SDP offer
    // to send to the client via your signaling mechanism.
    ClientId acceptConnection();

private:
    // Internal state for each connected client.
    struct ClientConnection {
        std::shared_ptr<rtc::PeerConnection> peerConnection;  // The WebRTC connection to this client
        std::shared_ptr<rtc::DataChannel> dataChannel;        // The actual data transport (like a WebSocket)
        std::string localDescription;                         // Cached SDP for signaling
        std::vector<std::string> pendingCandidates;           // ICE candidates received before DataChannel opens
        bool connected = false;
    };

    void run();

    void processOutgoing();

    void handleNewClient(ClientId clientId);

    void setupPeerConnectionCallbacks(ClientId clientId);
    void setupDataChannelCallbacks(ClientId clientId);

    const Game &m_game;

    std::atomic<bool> m_running{false};
    std::thread m_networkThread;
    Seconds m_tickPeriod{0.1f};

    std::mutex m_clientsMutex;
    std::unordered_map<ClientId, std::unique_ptr<ClientConnection>> m_clients;
    std::atomic<ClientId> m_nextClientId{1};

    // Tx message queue
    struct OutgoingMessage {
        ClientId clientId;
        std::vector<std::byte> data;
        bool broadcast;
    };
    std::mutex m_outgoingMutex;
    std::condition_variable m_outgoingCv;
    std::queue<OutgoingMessage> m_outgoingQueue;
};
#endif  // NETWORK_SERVER_H

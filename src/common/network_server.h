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
#include <chrono>

#include "game.h"

class NetworkServer {
public:
    using ClientId = uint32_t;
    using Seconds = std::chrono::duration<float, std::ratio<1>>;

    // Callbacks are invoked from the network thread, not the main thread.
    // Queue events if you need to process them on a different thread.
    using MessageCallback = std::function<void(ClientId, const std::vector<std::byte>&)>;
    using ConnectCallback = std::function<void(ClientId)>;
    using DisconnectCallback = std::function<void(ClientId)>;

    // Configuration for the WebRTC peer connections.
    // iceServers: STUN/TURN server URLs (e.g., "stun:stun.l.google.com:19302")
    // portRange: Optionally limit which local ports WebRTC uses (0 = any)
    struct Config {
        std::vector<std::string> iceServers;
        uint16_t portRangeBegin = 0;
        uint16_t portRangeEnd = 0;
    };

    NetworkServer(Config config, const Game &game);
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

    // -------------------------------------------------------------------------
    // SIGNALING METHODS
    // -------------------------------------------------------------------------
    // These are used to exchange connection information with the client.
    // You need to implement a separate transport (HTTP, WebSocket, etc.) to
    // actually send this data between server and client.
    //
    // Typical flow (server as offerer):
    //   1. Server: getLocalDescription(clientId) → SDP offer
    //   2. Send SDP offer to client via signaling channel
    //   3. Receive SDP answer from client → setRemoteDescription(clientId, answer)
    //   4. Exchange ICE candidates via setRemoteCandidate()
    // -------------------------------------------------------------------------

    // Returns the local SDP (offer or answer) for this client.
    // Empty string if not yet available (ICE gathering in progress).
    std::string getLocalDescription(ClientId clientId);

    // Set the remote SDP received from the client via signaling.
    void setRemoteDescription(ClientId clientId, const std::string& sdp);

    // Add a remote ICE candidate received from the client via signaling.
    // 'mid' is the media ID identifying which ICE stream this belongs to.
    void setRemoteCandidate(ClientId clientId, const std::string& candidate, const std::string& mid);

    void setMessageCallback(MessageCallback callback);
    void setConnectCallback(ConnectCallback callback);
    void setDisconnectCallback(DisconnectCallback callback);

private:
    // Internal state for each connected client.
    struct ClientConnection {
        std::shared_ptr<rtc::PeerConnection> peerConnection; // The WebRTC connection to this client
        std::shared_ptr<rtc::DataChannel> dataChannel; // The actual data transport (like a WebSocket)
        std::string localDescription; // Cached SDP for signaling
        std::vector<std::string> pendingCandidates; // ICE candidates received before DataChannel opens
        bool connected = false;
    };

    void run();

    void processOutgoing();

    void handleNewClient(ClientId clientId);

    void setupPeerConnectionCallbacks(ClientId clientId, ClientConnection& conn);
    void setupDataChannelCallbacks(ClientId clientId, ClientConnection& conn);

    // Configuration
    Config m_config;

    // Thread control.
    std::atomic<bool> m_running{false};
    std::thread m_networkThread;
    Seconds m_tickPeriod{0.1f};

    // Client state
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

    std::mutex m_callbackMutex;
    MessageCallback m_messageCallback;
    ConnectCallback m_connectCallback;
    DisconnectCallback m_disconnectCallback;

    // TODO need const ref to game (shared ptr? ref? c pointer?)
};
#endif  // NETWORK_SERVER_H

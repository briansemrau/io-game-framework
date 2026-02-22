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

class NetworkServer {
public:
    using ClientId = uint32_t;

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

    explicit NetworkServer(Config config);
    ~NetworkServer();

    NetworkServer(const NetworkServer&) = delete;
    NetworkServer& operator=(const NetworkServer&) = delete;
    NetworkServer(NetworkServer&&) = delete;
    NetworkServer& operator=(NetworkServer&&) = delete;

    // Start/stop the network thread. Call start() before accepting connections.
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

    // Set callbacks for connection events. Replace previous callback if any.
    void setMessageCallback(MessageCallback callback);
    void setConnectCallback(ConnectCallback callback);
    void setDisconnectCallback(DisconnectCallback callback);

private:
    // Internal state for each connected client.
    // peerConnection: The WebRTC connection to this client
    // dataChannel: The actual data transport (like a WebSocket)
    // localDescription: Cached SDP for signaling
    // pendingCandidates: ICE candidates received before DataChannel opens
    struct ClientConnection {
        std::shared_ptr<rtc::PeerConnection> peerConnection;
        std::shared_ptr<rtc::DataChannel> dataChannel;
        std::string localDescription;
        std::vector<std::string> pendingCandidates;
        bool connected = false;
    };

    // Main network thread loop. Processes WebRTC events and outgoing messages.
    void run();

    // Process all queued outgoing messages.
    void processOutgoing();

    // Create and configure a new PeerConnection for a client.
    void handleNewClient(ClientId clientId);

    // Set up callbacks on the PeerConnection (state changes, ICE gathering).
    void setupPeerConnectionCallbacks(ClientId clientId, ClientConnection& conn);

    // Set up callbacks on the DataChannel (open, message, close).
    void setupDataChannelCallbacks(ClientId clientId, ClientConnection& conn);

    // Configuration passed to constructor.
    Config m_config;

    // Network thread control.
    std::atomic<bool> m_running{false};
    std::thread m_networkThread;

    // Client state map, protected by mutex for thread safety.
    std::mutex m_clientsMutex;
    std::unordered_map<ClientId, std::unique_ptr<ClientConnection>> m_clients;
    std::atomic<ClientId> m_nextClientId{1};

    // Outgoing message queue. Main thread queues, network thread processes.
    struct OutgoingMessage {
        ClientId clientId;
        std::vector<std::byte> data;
        bool broadcast;
    };
    std::mutex m_outgoingMutex;
    std::condition_variable m_outgoingCv;
    std::queue<OutgoingMessage> m_outgoingQueue;

    // Callbacks, protected by mutex.
    std::mutex m_callbackMutex;
    MessageCallback m_messageCallback;
    ConnectCallback m_connectCallback;
    DisconnectCallback m_disconnectCallback;
};
#endif  // NETWORK_SERVER_H

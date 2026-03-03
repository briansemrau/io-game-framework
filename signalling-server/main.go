package main

import (
	"context"
	"encoding/json"
	"flag"
	"fmt"
	"log"
	"net/http"
	"os"
	"os/signal"
	"strings"
	"sync"
	"syscall"
	"time"

	"github.com/gorilla/websocket"
)

type PeerRole string

const (
	RoleClient PeerRole = "client"
	RoleServer PeerRole = "server"
)

type Message struct {
	ID          string   `json:"id"`
	Type        string   `json:"type"`
	Role        PeerRole `json:"role,omitempty"`
	Description string   `json:"description,omitempty"`
	Candidate   string   `json:"candidate,omitempty"`
	Mid         string   `json:"mid,omitempty"`
	Name        string   `json:"name,omitempty"`
	MaxPlayers  int      `json:"max_players,omitempty"`
	GameMode    string   `json:"game_mode,omitempty"`
}

const (
	MsgTypeOffer     = "offer"
	MsgTypeAnswer    = "answer"
	MsgTypeCandidate = "candidate"
	MsgTypeRegister  = "register"
)

func validateMessageType(t string) bool {
	switch t {
	case MsgTypeOffer, MsgTypeAnswer, MsgTypeCandidate, MsgTypeRegister:
		return true
	default:
		return false
	}
}

type PeerInfo struct {
	conn *websocket.Conn
	role PeerRole
}

type PeerManager struct {
	mu       sync.RWMutex
	peers    map[string]*PeerInfo
	servers  []string
	maxConns int
}

func NewPeerManager(maxConns int) *PeerManager {
	log.Printf("Creating new PeerManager with maxConns=%d", maxConns)
	return &PeerManager{
		peers:    make(map[string]*PeerInfo),
		servers:  make([]string, 0),
		maxConns: maxConns,
	}
}

func (pm *PeerManager) Register(id string, c *websocket.Conn, role PeerRole) error {
	pm.mu.Lock()
	defer pm.mu.Unlock()

	log.Printf("Attempting to register %s id=%s, current connections=%d", role, id, len(pm.peers))

	if _, exists := pm.peers[id]; exists {
		log.Printf("REJECTED: ID already in use id=%s", id)
		// TODO: Implement retry mechanism - client should generate new ID and reconnect on this error
		return os.ErrExist
	}

	if pm.maxConns > 0 && len(pm.peers) >= pm.maxConns {
		log.Printf("REJECTED: max connections reached (%d)", pm.maxConns)
		return os.ErrExist
	}

	pm.peers[id] = &PeerInfo{conn: c, role: role}
	if role == RoleServer {
		pm.servers = append(pm.servers, id)
		log.Printf("Added server to list: %v", pm.servers)
	}
	log.Printf("SUCCESS: registered %s id=%s, total connections=%d", role, id, len(pm.peers))
	return nil
}

func (pm *PeerManager) Unregister(id string) {
	pm.mu.Lock()
	defer pm.mu.Unlock()

	log.Printf("Attempting to unregister id=%s", id)

	if peer, ok := pm.peers[id]; ok {
		delete(pm.peers, id)
		peer.conn.Close()
		// Remove from servers list if it was a server
		if peer.role == RoleServer {
			for i, serverID := range pm.servers {
				if serverID == id {
					pm.servers = append(pm.servers[:i], pm.servers[i+1:]...)
					log.Printf("Removed server from list: %v", pm.servers)
					break
				}
			}
		}
		log.Printf("SUCCESS: unregistered id=%s, remaining connections=%d", id, len(pm.peers))
	} else {
		log.Printf("WARNING: Attempted to unregister nonexistent id: %s", id)
	}
}

func (pm *PeerManager) Get(id string) (*PeerInfo, bool) {
	pm.mu.RLock()
	defer pm.mu.RUnlock()

	peer, ok := pm.peers[id]
	log.Printf("Looking up id=%s, found=%v", id, ok)
	return peer, ok
}

func (pm *PeerManager) Len() int {
	pm.mu.RLock()
	defer pm.mu.RUnlock()
	return len(pm.peers)
}

func (pm *PeerManager) GetRole(id string) PeerRole {
	pm.mu.RLock()
	defer pm.mu.RUnlock()

	if peer, ok := pm.peers[id]; ok {
		return peer.role
	}
	return ""
}

func (pm *PeerManager) GetServerList() []string {
	pm.mu.RLock()
	defer pm.mu.RUnlock()

	// Return a copy to avoid race conditions
	servers := make([]string, len(pm.servers))
	copy(servers, pm.servers)
	return servers
}

func newUpgrader() websocket.Upgrader {
	allowedOrigins := strings.Split(os.Getenv("ALLOWED_ORIGINS"), ",")
	log.Printf("ALLOWED_ORIGINS env var: '%s'", os.Getenv("ALLOWED_ORIGINS"))
	log.Printf("Parsed allowed origins count: %d", len(allowedOrigins))

	if len(allowedOrigins) == 0 || (len(allowedOrigins) == 1 && allowedOrigins[0] == "") {
		log.Printf("Using permissive origin check (all origins allowed)")
		return websocket.Upgrader{
			CheckOrigin:     func(r *http.Request) bool { return true },
			ReadBufferSize:  0, // use default
			WriteBufferSize: 0,
		}
	}
	log.Printf("Using restrictive origin check, allowed: %v", allowedOrigins)
	return websocket.Upgrader{
		ReadBufferSize:  0, // use default
		WriteBufferSize: 0,
		CheckOrigin: func(r *http.Request) bool {
			origin := r.Header.Get("Origin")
			log.Printf("Checking origin: '%s'", origin)
			for _, o := range allowedOrigins {
				if o == origin {
					log.Printf("Origin '%s' ALLOWED", origin)
					return true
				}
			}
			log.Printf("Origin '%s' DENIED", origin)
			return false
		},
	}
}

var upgrader = newUpgrader()

func wsHandler(pm *PeerManager) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		log.Printf("Incoming request: method=%s url=%s remoteAddr=%s", r.Method, r.URL.Path, r.RemoteAddr)

		id := strings.TrimPrefix(r.URL.Path, "/connect/")
		if id == "" || id == r.URL.Path {
			log.Printf("REJECTED: missing id in path %s", r.URL.Path)
			http.Error(w, "missing id", http.StatusBadRequest)
			return
		}
		log.Printf("Extracted id from path: %s", id)

		log.Printf("Attempting WebSocket upgrade for id=%s", id)
		conn, err := upgrader.Upgrade(w, r, nil)
		if err != nil {
			log.Printf("Upgrade FAILED for id=%s: %v", id, err)
			http.Error(w, "upgrade failed", http.StatusInternalServerError)
			return
		}
		log.Printf("Upgrade SUCCESS for id=%s", id)

		registered := false

		defer func() {
			log.Printf("Closing WebSocket connection for id=%s", id)
			if registered {
				pm.Unregister(id)
			}
			conn.Close()
		}()

		conn.SetPongHandler(func(appData string) error {
			log.Printf("Pong received from id=%s, appData=%q", id, appData)
			return nil
		})
		conn.SetPingHandler(func(appData string) error {
			log.Printf("Ping received from id=%s, appData=%q", id, appData)
			return conn.WriteMessage(websocket.PongMessage, nil)
		})

		log.Printf("Connection established for id=%s, waiting for registration", id)

		for {
			msgType, data, err := conn.ReadMessage()
			if err != nil {
				if websocket.IsUnexpectedCloseError(err, websocket.CloseGoingAway, websocket.CloseAbnormalClosure) {
					log.Printf("Read ERROR for id=%s: %v", id, err)
				} else {
					log.Printf("Read closed for id=%s: %v", id, err)
				}
				return
			}
			log.Printf("Received message from id=%s, type=%d, size=%d bytes", id, msgType, len(data))

			var msg Message
			if err := json.Unmarshal(data, &msg); err != nil {
				log.Printf("JSON parse ERROR for id=%s: %v, raw data: %s", id, err, string(data))
				continue
			}
			log.Printf("Parsed message from id=%s: type=%s, role=%s, targetID=%s", id, msg.Type, msg.Role, msg.ID)

			if !validateMessageType(msg.Type) {
				log.Printf("INVALID message type from id=%s: %s", id, msg.Type)
				continue
			}

			// Handle registration message first
			if msg.Type == MsgTypeRegister {
				if registered {
					log.Printf("REJECTED: id=%s already registered", id)
					continue
				}
				if msg.Role != RoleClient && msg.Role != RoleServer {
					log.Printf("REJECTED: invalid role '%s' from id=%s", msg.Role, id)
					continue
				}
				if err := pm.Register(id, conn, msg.Role); err != nil {
					log.Printf("Registration FAILED for id=%s: %v", id, err)
					// TODO: Implement retry mechanism for ID conflicts - client should generate new ID and reconnect
					continue
				}
				registered = true
				log.Printf("Registration SUCCESS for id=%s as %s", id, msg.Role)
				if msg.Role == RoleServer {
					log.Printf("Server metadata: name=%s, max_players=%d, game_mode=%s", msg.Name, msg.MaxPlayers, msg.GameMode)
				}
				continue
			}

			// All other messages require registration first
			if !registered {
				log.Printf("REJECTED: id=%s not yet registered, ignoring %s", id, msg.Type)
				continue
			}

			targetID := msg.ID
			if targetID == "" {
				targetID = id
				log.Printf("No target ID specified, defaulting to sender id=%s", id)
			} else {
				log.Printf("Routing message to target id=%s", targetID)
			}

			targetPeer, ok := pm.Get(targetID)
			if !ok {
				log.Printf("Target NOT FOUND: id=%s -> targetID=%s", id, targetID)
				continue
			}

			// For offers: block client-to-client connections
			if msg.Type == MsgTypeOffer {
				senderRole := pm.GetRole(id)
				targetRole := targetPeer.role
				if senderRole == RoleClient && targetRole == RoleClient {
					log.Printf("REJECTED: client-to-client offer from id=%s to targetID=%s", id, targetID)
					continue
				}
				log.Printf("ALLOWED: %s-to-%s offer from id=%s to targetID=%s", senderRole, targetRole, id, targetID)
			}

			log.Printf("Target FOUND: forwarding message to id=%s", targetID)

			msg.ID = id
			data, err = json.Marshal(msg)
			if err != nil {
				log.Printf("JSON marshal ERROR for id=%s: %v, msg: %s", id, err, msg)
				continue
			}
			if err := targetPeer.conn.WriteMessage(websocket.TextMessage, data); err != nil {
				log.Printf("Write ERROR to target id=%s: %v", targetID, err)
				return
			}
			log.Printf("Successfully forwarded message from id=%s to targetID=%s (%d bytes)", id, targetID, len(data))
		}
	}
}

func main() {
	addr := flag.String("addr", ":8080", "host:port for signalling server")
	maxConns := flag.Int("max-connections", 0, "maximum concurrent connections (0 = unlimited)")
	tls := flag.Bool("tls", false, "enable TLS")
	cert := flag.String("cert", "", "path to TLS certificate file")
	key := flag.String("key", "", "path to TLS key file")
	flag.Parse()

	log.Printf("Starting signalling server")
	log.Printf("Configuration: addr=%s, maxConns=%d, tls=%v", *addr, *maxConns, *tls)
	if *tls {
		log.Printf("TLS Configuration: cert=%s, key=%s", *cert, *key)
	}

	pm := NewPeerManager(*maxConns)

	http.HandleFunc("/health", func(w http.ResponseWriter, r *http.Request) {
		log.Printf("/health check request from %s", r.RemoteAddr)
		w.WriteHeader(http.StatusOK)
		w.Write([]byte("OK"))
	})

	http.HandleFunc("/stats", func(w http.ResponseWriter, r *http.Request) {
		log.Printf("/stats request from %s", r.RemoteAddr)
		w.Header().Set("Content-Type", "application/json")
		w.WriteHeader(http.StatusOK)
		fmt.Fprintf(w, `{"connections":%d}`, pm.Len())
	})

	http.HandleFunc("/servers", func(w http.ResponseWriter, r *http.Request) {
		log.Printf("/servers request from %s", r.RemoteAddr)
		w.Header().Set("Content-Type", "application/json")
		w.WriteHeader(http.StatusOK)
		json.NewEncoder(w).Encode(pm.GetServerList())
	})

	http.HandleFunc("/connect/", wsHandler(pm))
	log.Printf("Registered HTTP handler: /connect/")

	srv := &http.Server{Addr: *addr}

	go func() {
		sig := make(chan os.Signal, 1)
		signal.Notify(sig, os.Interrupt, syscall.SIGTERM)
		<-sig

		log.Printf("Received shutdown signal (SIGINT/SIGTERM)")
		log.Printf("Initiating graceful shutdown...")

		ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
		defer cancel()

		pm.mu.Lock()
		connCount := len(pm.peers)
		log.Printf("Shutting down %d active connections", connCount)
		for id, peer := range pm.peers {
			log.Printf("Closing connection for id=%s", id)
			peer.conn.WriteControl(websocket.CloseMessage, websocket.FormatCloseMessage(websocket.CloseGoingAway, "server shutting down"), time.Now().Add(time.Second))
			peer.conn.Close()
			delete(pm.peers, id)
		}
		pm.mu.Unlock()

		if err := srv.Shutdown(ctx); err != nil {
			log.Printf("Server shutdown error: %v", err)
		} else {
			log.Printf("Server shutdown completed successfully")
		}
	}()

	log.Printf("Signalling server listening on %s", *addr)

	var err error
	if *tls {
		if *cert == "" || *key == "" {
			log.Fatal("[main] TLS requires both -cert and -key flags")
		}
		log.Printf("Starting HTTPS server with TLS")
		err = srv.ListenAndServeTLS(*cert, *key)
	} else {
		log.Printf("Starting HTTP server")
		err = srv.ListenAndServe()
	}
	if err != nil && err != http.ErrServerClosed {
		log.Fatalf("[main] Listen failed: %v", err)
	}
	log.Printf("Server stopped")
}

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
	// WebRTC signalling
	MsgTypeOffer     = "offer"
	MsgTypeAnswer    = "answer"
	MsgTypeCandidate = "candidate"
	// Client/Server metadata
	MsgTypeRegister = "register"
	MsgTypeJoin     = "join"
)

func validateMessageType(t string) bool {
	switch t {
	case MsgTypeOffer, MsgTypeAnswer, MsgTypeCandidate, MsgTypeRegister, MsgTypeJoin:
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
	return &PeerManager{
		peers:    make(map[string]*PeerInfo),
		servers:  make([]string, 0),
		maxConns: maxConns,
	}
}

func (pm *PeerManager) Register(id string, c *websocket.Conn, role PeerRole) error {
	pm.mu.Lock()
	defer pm.mu.Unlock()

	if _, exists := pm.peers[id]; exists {
		log.Printf("Error: ID already in use (id=%s)", id)
		return os.ErrExist
	}

	if pm.maxConns > 0 && len(pm.peers) >= pm.maxConns {
		log.Printf("Error: max connections reached (%d), rejecting id: %s", pm.maxConns, id)
		return os.ErrExist
	}

	pm.peers[id] = &PeerInfo{conn: c, role: role}
	if role == RoleServer {
		pm.servers = append(pm.servers, id)
	}
	log.Printf("Registered %s with id=%s (total connections=%d)", role, id, len(pm.peers))
	return nil
}

func (pm *PeerManager) Unregister(id string) {
	pm.mu.Lock()
	defer pm.mu.Unlock()

	if peer, ok := pm.peers[id]; ok {
		delete(pm.peers, id)
		peer.conn.Close()
		if peer.role == RoleServer {
			removed := false
			for i, serverID := range pm.servers {
				if serverID == id {
					pm.servers = append(pm.servers[:i], pm.servers[i+1:]...)
					removed = true
					break
				}
			}
			if !removed {
				log.Printf("Error: failed to find and remove server with id=%s")
			}
		}
	} else {
		log.Printf("Error: Attempted to unregister nonexistent id: %s", id)
	}
}

func (pm *PeerManager) Get(id string) (*PeerInfo, bool) {
	pm.mu.RLock()
	defer pm.mu.RUnlock()
	peer, ok := pm.peers[id]
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
	servers := make([]string, len(pm.servers))
	copy(servers, pm.servers)
	return servers
}

func newUpgrader() websocket.Upgrader {
	allowedOrigins := strings.Split(os.Getenv("ALLOWED_ORIGINS"), ",")

	if len(allowedOrigins) == 0 || (len(allowedOrigins) == 1 && allowedOrigins[0] == "") {
		log.Printf("Using permissive origin check (all origins allowed)")
		return websocket.Upgrader{
			CheckOrigin:     func(r *http.Request) bool { return true },
			ReadBufferSize:  0,
			WriteBufferSize: 0,
		}
	}
	log.Printf("Using restrictive origin check, allowed: %v", allowedOrigins)
	return websocket.Upgrader{
		ReadBufferSize:  0,
		WriteBufferSize: 0,
		CheckOrigin: func(r *http.Request) bool {
			origin := r.Header.Get("Origin")
			for _, o := range allowedOrigins {
				if o == origin {
					return true
				}
			}
			return false
		},
	}
}

var upgrader = newUpgrader()

func wsHandler(pm *PeerManager) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		id := strings.TrimPrefix(r.URL.Path, "/connect/")
		if id == "" || id == r.URL.Path {
			http.Error(w, "missing id", http.StatusBadRequest)
			return
		}

		conn, err := upgrader.Upgrade(w, r, nil)
		if err != nil {
			http.Error(w, "upgrade failed", http.StatusInternalServerError)
			return
		}

		registered := false
		defer func() {
			if registered {
				pm.Unregister(id)
			}
			conn.Close()
		}()

		conn.SetPongHandler(func(appData string) error { return nil })
		conn.SetPingHandler(func(appData string) error { return conn.WriteMessage(websocket.PongMessage, nil) })

		handleConnection(pm, id, conn, &registered)
	}
}

func handleConnection(pm *PeerManager, id string, conn *websocket.Conn, registered *bool) {
	for {
		_, data, err := conn.ReadMessage()
		if err != nil {
			if websocket.IsUnexpectedCloseError(err, websocket.CloseGoingAway, websocket.CloseAbnormalClosure) {
				log.Printf("Read error for id=%s: %v", id, err)
			} else {
				log.Printf("Read closed for id=%s: %v", id, err)
			}
			return
		}

		var msg Message
		if err := json.Unmarshal(data, &msg); err != nil {
			continue
		}

		if !validateMessageType(msg.Type) {
			continue
		}

		if msg.Type == MsgTypeRegister {
			if *registered {
				continue
			}
			if msg.Role != RoleClient && msg.Role != RoleServer {
				continue
			}
			if err := pm.Register(id, conn, msg.Role); err != nil {
				continue
			}
			*registered = true
			continue
		}

		if !*registered {
			continue
		}

		targetID := msg.ID
		if targetID == "" {
			targetID = id
		}

		targetPeer, ok := pm.Get(targetID)
		if !ok {
			continue
		}

		if msg.Type == MsgTypeOffer {
			senderRole := pm.GetRole(id)
			if senderRole == RoleClient && targetPeer.role == RoleClient {
				continue
			}
		}

		msg.ID = id
		data, err = json.Marshal(msg)
		if err != nil {
			continue
		}
		if err := targetPeer.conn.WriteMessage(websocket.TextMessage, data); err != nil {
			return
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
		w.WriteHeader(http.StatusOK)
		w.Write([]byte("OK"))
	})

	http.HandleFunc("/stats", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")
		w.WriteHeader(http.StatusOK)
		fmt.Fprintf(w, `{"connections":%d}`, pm.Len())
	})

	http.HandleFunc("/servers", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")
		w.WriteHeader(http.StatusOK)
		json.NewEncoder(w).Encode(pm.GetServerList())
	})

	http.HandleFunc("/connect/", wsHandler(pm))

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
		for _, peer := range pm.peers {
			peer.conn.WriteControl(websocket.CloseMessage, websocket.FormatCloseMessage(websocket.CloseGoingAway, "server shutting down"), time.Now().Add(time.Second))
			peer.conn.Close()
		}
		pm.mu.Unlock()

		srv.Shutdown(ctx)
	}()

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

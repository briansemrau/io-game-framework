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

type Message struct {
	ID          string `json:"id"`
	Type        string `json:"type"`
	Description string `json:"description,omitempty"`
	Candidate   string `json:"candidate,omitempty"`
	Mid         string `json:"mid,omitempty"`
}

const (
	MsgTypeOffer        = "offer"
	MsgTypeAnswer       = "answer"
	MsgTypeICECandidate = "ice-candidate"
)

func validateMessageType(t string) bool {
	switch t {
	case MsgTypeOffer, MsgTypeAnswer, MsgTypeICECandidate:
		return true
	default:
		return false
	}
}

type PeerManager struct {
	mu       sync.RWMutex
	conns    map[string]*websocket.Conn
	maxConns int
}

func NewPeerManager(maxConns int) *PeerManager {
	log.Printf("[PeerManager] Creating new PeerManager with maxConns=%d", maxConns)
	return &PeerManager{
		conns:    make(map[string]*websocket.Conn),
		maxConns: maxConns,
	}
}

func (pm *PeerManager) Register(id string, c *websocket.Conn) error {
	pm.mu.Lock()
	defer pm.mu.Unlock()

	log.Printf("[PeerManager.Register] Attempting to register client id=%s, current connections=%d", id, len(pm.conns))

	if pm.maxConns > 0 && len(pm.conns) >= pm.maxConns {
		log.Printf("[PeerManager.Register] REJECTED: max connections reached (%d)", pm.maxConns)
		return os.ErrExist
	}

	pm.conns[id] = c
	log.Printf("[PeerManager.Register] SUCCESS: registered client id=%s, total connections=%d", id, len(pm.conns))
	return nil
}

func (pm *PeerManager) Unregister(id string) {
	pm.mu.Lock()
	defer pm.mu.Unlock()

	log.Printf("[PeerManager.Unregister] Attempting to unregister client id=%s", id)

	if conn, ok := pm.conns[id]; ok {
		delete(pm.conns, id)
		conn.Close()
		log.Printf("[PeerManager.Unregister] SUCCESS: unregistered client id=%s, remaining connections=%d", id, len(pm.conns))
	} else {
		log.Printf("[PeerManager.Unregister] WARNING: Attempted to unregister nonexistent id: %s", id)
	}
}

func (pm *PeerManager) Get(id string) (*websocket.Conn, bool) {
	pm.mu.RLock()
	defer pm.mu.RUnlock()

	conn, ok := pm.conns[id]
	log.Printf("[PeerManager.Get] Looking up client id=%s, found=%v", id, ok)
	return conn, ok
}

func (pm *PeerManager) Len() int {
	pm.mu.RLock()
	defer pm.mu.RUnlock()
	return len(pm.conns)
}

func newUpgrader() websocket.Upgrader {
	allowedOrigins := strings.Split(os.Getenv("ALLOWED_ORIGINS"), ",")
	log.Printf("[WebSocket.Upgrader] ALLOWED_ORIGINS env var: '%s'", os.Getenv("ALLOWED_ORIGINS"))
	log.Printf("[WebSocket.Upgrader] Parsed allowed origins count: %d", len(allowedOrigins))

	if len(allowedOrigins) == 0 || (len(allowedOrigins) == 1 && allowedOrigins[0] == "") {
		log.Printf("[WebSocket.Upgrader] Using permissive origin check (all origins allowed)")
		return websocket.Upgrader{
			CheckOrigin:     func(r *http.Request) bool { return true },
			ReadBufferSize:  0, // use default
			WriteBufferSize: 0,
		}
	}
	log.Printf("[WebSocket.Upgrader] Using restrictive origin check, allowed: %v", allowedOrigins)
	return websocket.Upgrader{
		ReadBufferSize:  0, // use default
		WriteBufferSize: 0,
		CheckOrigin: func(r *http.Request) bool {
			origin := r.Header.Get("Origin")
			log.Printf("[WebSocket.Upgrader] Checking origin: '%s'", origin)
			for _, o := range allowedOrigins {
				if o == origin {
					log.Printf("[WebSocket.Upgrader] Origin '%s' ALLOWED", origin)
					return true
				}
			}
			log.Printf("[WebSocket.Upgrader] Origin '%s' DENIED", origin)
			return false
		},
	}
}

var upgrader = newUpgrader()

func wsHandler(pm *PeerManager) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		log.Printf("[wsHandler] Incoming request: method=%s url=%s remoteAddr=%s", r.Method, r.URL.Path, r.RemoteAddr)

		id := r.URL.Path[1:]
		if id == "" {
			log.Printf("[wsHandler] REJECTED: missing client id in path %s", r.URL.Path)
			http.Error(w, "missing client id", http.StatusBadRequest)
			return
		}
		log.Printf("[wsHandler] Extracted client id from path: %s", id)

		log.Printf("[wsHandler] Attempting WebSocket upgrade for client id=%s", id)
		conn, err := upgrader.Upgrade(w, r, nil)
		if err != nil {
			log.Printf("[wsHandler] Upgrade FAILED for client id=%s: %v", id, err)
			http.Error(w, "upgrade failed", http.StatusInternalServerError)
			return
		}
		log.Printf("[wsHandler] Upgrade SUCCESS for client id=%s", id)

		defer func() {
			log.Printf("[wsHandler] Closing WebSocket connection for client id=%s", id)
			conn.Close()
		}()

		if err := pm.Register(id, conn); err != nil {
			log.Printf("[wsHandler] Registration FAILED for client id=%s: %v", id, err)
			return
		}
		defer pm.Unregister(id)

		conn.SetPongHandler(func(appData string) error {
			log.Printf("[wsHandler] Pong received from client id=%s, appData=%q", id, appData)
			return nil
		})
		conn.SetPingHandler(func(appData string) error {
			log.Printf("[wsHandler] Ping received from client id=%s, appData=%q", id, appData)
			return conn.WriteMessage(websocket.PongMessage, nil)
		})

		log.Printf("[wsHandler] Client %s fully connected and registered", id)

		for {
			msgType, data, err := conn.ReadMessage()
			if err != nil {
				if websocket.IsUnexpectedCloseError(err, websocket.CloseGoingAway, websocket.CloseAbnormalClosure) {
					log.Printf("[wsHandler] Read ERROR for client id=%s: %v", id, err)
				} else {
					log.Printf("[wsHandler] Read closed for client id=%s: %v", id, err)
				}
				return
			}
			log.Printf("[wsHandler] Received message from client id=%s, type=%d, size=%d bytes", id, msgType, len(data))

			var msg Message
			if err := json.Unmarshal(data, &msg); err != nil {
				log.Printf("[wsHandler] JSON parse ERROR for client id=%s: %v, raw data: %s", id, err, string(data))
				continue
			}
			log.Printf("[wsHandler] Parsed message from id=%s: type=%s, targetID=%s, description=%s, candidate=%s, mid=%s",
				id, msg.Type, msg.ID, msg.Description, msg.Candidate, msg.Mid)

			if !validateMessageType(msg.Type) {
				log.Printf("[wsHandler] INVALID message type from id=%s: %s", id, msg.Type)
				continue
			}
			log.Printf("[wsHandler] Message type %s is VALID", msg.Type)

			targetID := msg.ID
			if targetID == "" {
				targetID = id
				log.Printf("[wsHandler] No target ID specified, defaulting to sender id=%s", id)
			} else {
				log.Printf("[wsHandler] Routing message to target id=%s", targetID)
			}

			targetConn, ok := pm.Get(targetID)
			if !ok {
				log.Printf("[wsHandler] Target client NOT FOUND: id=%s -> targetID=%s", id, targetID)
				continue
			}
			log.Printf("[wsHandler] Target client FOUND: forwarding message to id=%s", targetID)

			if err := targetConn.WriteMessage(websocket.TextMessage, data); err != nil {
				log.Printf("[wsHandler] Write ERROR to target id=%s: %v", targetID, err)
				return
			}
			log.Printf("[wsHandler] Successfully forwarded message from id=%s to targetID=%s (%d bytes)", id, targetID, len(data))
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

	log.Printf("[main] Starting signalling server")
	log.Printf("[main] Configuration: addr=%s, maxConns=%d, tls=%v", *addr, *maxConns, *tls)
	if *tls {
		log.Printf("[main] TLS Configuration: cert=%s, key=%s", *cert, *key)
	}

	pm := NewPeerManager(*maxConns)

	http.HandleFunc("/health", func(w http.ResponseWriter, r *http.Request) {
		log.Printf("[HTTP] /health check request from %s", r.RemoteAddr)
		w.WriteHeader(http.StatusOK)
		w.Write([]byte("OK"))
	})

	http.HandleFunc("/stats", func(w http.ResponseWriter, r *http.Request) {
		log.Printf("[HTTP] /stats request from %s", r.RemoteAddr)
		w.Header().Set("Content-Type", "application/json")
		w.WriteHeader(http.StatusOK)
		fmt.Fprintf(w, `{"connections":%d}`, pm.Len())
	})

	http.HandleFunc("/connect/", wsHandler(pm))
	log.Printf("[main] Registered HTTP handler: /connect/")

	srv := &http.Server{Addr: *addr}

	go func() {
		sig := make(chan os.Signal, 1)
		signal.Notify(sig, os.Interrupt, syscall.SIGTERM)
		<-sig

		log.Printf("[main] Received shutdown signal (SIGINT/SIGTERM)")
		log.Printf("[main] Initiating graceful shutdown...")

		ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
		defer cancel()

		pm.mu.Lock()
		connCount := len(pm.conns)
		log.Printf("[main] Shutting down %d active connections", connCount)
		for id, conn := range pm.conns {
			log.Printf("[main] Closing connection for client id=%s", id)
			conn.WriteControl(websocket.CloseMessage, websocket.FormatCloseMessage(websocket.CloseGoingAway, "server shutting down"), time.Now().Add(time.Second))
			conn.Close()
			delete(pm.conns, id)
		}
		pm.mu.Unlock()

		if err := srv.Shutdown(ctx); err != nil {
			log.Printf("[main] Server shutdown error: %v", err)
		} else {
			log.Printf("[main] Server shutdown completed successfully")
		}
	}()

	log.Printf("[main] Signalling server listening on %s", *addr)

	var err error
	if *tls {
		if *cert == "" || *key == "" {
			log.Fatal("[main] TLS requires both -cert and -key flags")
		}
		log.Printf("[main] Starting HTTPS server with TLS")
		err = srv.ListenAndServeTLS(*cert, *key)
	} else {
		log.Printf("[main] Starting HTTP server")
		err = srv.ListenAndServe()
	}
	if err != nil && err != http.ErrServerClosed {
		log.Fatalf("[main] Listen failed: %v", err)
	}
	log.Printf("[main] Server stopped")
}

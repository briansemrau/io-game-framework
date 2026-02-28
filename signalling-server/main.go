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
	return &PeerManager{
		conns:    make(map[string]*websocket.Conn),
		maxConns: maxConns,
	}
}

func (pm *PeerManager) Register(id string, c *websocket.Conn) error {
	pm.mu.Lock()
	defer pm.mu.Unlock()

	if pm.maxConns > 0 && len(pm.conns) >= pm.maxConns {
		return os.ErrExist
	}

	pm.conns[id] = c
	return nil
}

func (pm *PeerManager) Unregister(id string) {
	pm.mu.Lock()
	defer pm.mu.Unlock()

	if conn, ok := pm.conns[id]; ok {
		delete(pm.conns, id)
		conn.Close()
	} else {
		log.Printf("Attempted to unregister nonexistent id: %s", id)
	}
}

func (pm *PeerManager) Get(id string) (*websocket.Conn, bool) {
	pm.mu.RLock()
	defer pm.mu.RUnlock()

	conn, ok := pm.conns[id]
	return conn, ok
}

func (pm *PeerManager) Len() int {
	pm.mu.RLock()
	defer pm.mu.RUnlock()
	return len(pm.conns)
}

func newUpgrader() websocket.Upgrader {
	allowedOrigins := strings.Split(os.Getenv("ALLOWED_ORIGINS"), ",")
	if len(allowedOrigins) == 0 || (len(allowedOrigins) == 1 && allowedOrigins[0] == "") {
		return websocket.Upgrader{
			CheckOrigin:     func(r *http.Request) bool { return true },
			ReadBufferSize:  0, // use default
			WriteBufferSize: 0,
		}
	}
	return websocket.Upgrader{
		ReadBufferSize:  0, // use default
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
		id := r.URL.Path[1:]
		if id == "" {
			http.Error(w, "missing client id", http.StatusBadRequest)
			return
		}

		conn, err := upgrader.Upgrade(w, r, nil)
		if err != nil {
			log.Printf("upgrade error for %s: %v", id, err)
			http.Error(w, "upgrade failed", http.StatusInternalServerError)
			return
		}

		defer conn.Close()
		if err := pm.Register(id, conn); err != nil {
			log.Printf("registration failed for %s: %v", id, err)
			return
		}
		defer pm.Unregister(id)

		conn.SetPongHandler(func(string) error { return nil })
		conn.SetPingHandler(func(string) error {
			return conn.WriteMessage(websocket.PongMessage, nil)
		})

		log.Printf("client %s connected", id)

		for {
			_, data, err := conn.ReadMessage()
			if err != nil {
				if websocket.IsUnexpectedCloseError(err, websocket.CloseGoingAway, websocket.CloseAbnormalClosure) {
					log.Printf("read error (%s): %v", id, err)
				}
				return
			}

			var msg Message
			if err := json.Unmarshal(data, &msg); err != nil {
				log.Printf("json error (%s): %v", id, err)
				continue
			}

			if !validateMessageType(msg.Type) {
				log.Printf("invalid message type (%s): %s", id, msg.Type)
				continue
			}

			targetID := msg.ID
			if targetID == "" {
				targetID = id
			}

			conn, ok := pm.Get(targetID)
			if !ok {
				log.Printf("target not found (%s -> %s)", id, targetID)
				continue
			}

			if err := conn.WriteMessage(websocket.TextMessage, data); err != nil {
				log.Printf("write error (%s): %v", targetID, err)
				return
			}
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

	http.HandleFunc("/connect/", wsHandler(pm))

	srv := &http.Server{Addr: *addr}

	go func() {
		sig := make(chan os.Signal, 1)
		signal.Notify(sig, os.Interrupt, syscall.SIGTERM)
		<-sig

		log.Println("shutting down...")

		ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
		defer cancel()

		pm.mu.Lock()
		for id, conn := range pm.conns {
			conn.WriteControl(websocket.CloseMessage, websocket.FormatCloseMessage(websocket.CloseGoingAway, "server shutting down"), time.Now().Add(time.Second))
			conn.Close()
			delete(pm.conns, id)
		}
		pm.mu.Unlock()

		if err := srv.Shutdown(ctx); err != nil {
			log.Printf("server shutdown error: %v", err)
		}
	}()

	log.Printf("signalling server listening on %s", *addr)

	var err error
	if *tls {
		if *cert == "" || *key == "" {
			log.Fatal("TLS requires both -cert and -key flags")
		}
		err = srv.ListenAndServeTLS(*cert, *key)
	} else {
		err = srv.ListenAndServe()
	}
	if err != nil && err != http.ErrServerClosed {
		log.Fatalf("listen failed: %v", err)
	}
}

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

type PeerManager struct {
	mu       sync.RWMutex
	conns    map[string]*websocket.Conn
	maxConns int
	current  int
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

	if _, ok := pm.conns[id]; ok {
		pm.current--
	} else if pm.maxConns > 0 && pm.current >= pm.maxConns {
		return os.ErrExist
	}

	pm.conns[id] = c
	pm.current++
	return nil
}

func (pm *PeerManager) Unregister(id string) {
	pm.mu.Lock()
	defer pm.mu.Unlock()

	if conn, ok := pm.conns[id]; ok {
		delete(pm.conns, id)
		pm.current--
		conn.Close()
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
	return pm.current
}

func newUpgrader() websocket.Upgrader {
	allowedOrigins := strings.Split(os.Getenv("ALLOWED_ORIGINS"), ",")
	if len(allowedOrigins) == 0 || (len(allowedOrigins) == 1 && allowedOrigins[0] == "") {
		return websocket.Upgrader{
			CheckOrigin:     func(r *http.Request) bool { return true },
			ReadBufferSize:  1024,
			WriteBufferSize: 1024,
		}
	}
	return websocket.Upgrader{
		ReadBufferSize:  1024,
		WriteBufferSize: 1024,
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

		if err := pm.Register(id, nil); err != nil {
			http.Error(w, "server at capacity", http.StatusTooManyRequests)
			return
		}

		conn, err := upgrader.Upgrade(w, r, nil)
		if err != nil {
			pm.Unregister(id)
			log.Printf("upgrade error for %s: %v", id, err)
			return
		}

		defer conn.Close()
		pm.Register(id, conn)
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

			if msg.Type == "" {
				log.Printf("missing type (%s)", id)
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

	http.HandleFunc("/", wsHandler(pm))

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
	if err := srv.ListenAndServe(); err != nil && err != http.ErrServerClosed {
		log.Fatalf("listen failed: %v", err)
	}
}

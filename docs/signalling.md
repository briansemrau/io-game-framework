# Signalling Server Architecture

## Overview

The signalling server is a Go-based WebSocket relay that facilitates WebRTC connections between game clients and game servers. It uses a role-based registration system to control which peers can connect to each other.

**Key properties:**
- Game servers are publicly discoverable via HTTP `/servers` endpoint
- Clients are hidden (never listed)
- Client-to-client connections are blocked
- Only client-to-server and server-to-client connections are allowed
- WebRTC data channels provide low-latency transport after signalling

## Architecture Diagram

```mermaid
graph TD
    subgraph Server
        GS[Game Server]
    end

    subgraph Client
        C[Game Client]
    end

    subgraph Signalling
        S[Signalling Server<br/>WebSocket + HTTP]
        SR[(Server Registry)]
    end

    GS -->|WebSocket| S
    C -->|WebSocket| S
    S --> SR
    GS -.->|WebRTC Data Channel Negotiation| C
```

## Registration Protocol

### Message Format

All messages are JSON-encoded:

**Registration (first message after WebSocket connect):**
```json
{
  "type": "register",
  "id": "unique-peer-id",
  "role": "client" | "server",
  "name": "server-name",         // optional, server only
  "max_players": 8,              // optional, server only
  "game_mode": "deathmatch"      // optional, server only
}
```

**Offer (initiate connection):**
```json
{
  "type": "offer",
  "target_id": "peer-id-to-connect-to",
  "sdp": "session-description-protocol-string"
}
```

**Answer (respond to offer):**
```json
{
  "type": "answer",
  "target_id": "original-offer-sender",
  "sdp": "session-description-protocol-string"
}
```

### Connection Flow

```mermaid
sequenceDiagram
    participant C as Client
    participant S as Signalling Server
    participant G as Game Server

    Note over C,G: Phase 1: Server Registration
    G->>S: WebSocket connect
    G->>S: register { role: "server", name: "MyServer", max_players: 8 }
    S-->>G: register_ok
    Note right of S: Server added to /servers list

    Note over C,G: Phase 2: Client Registration & Discovery
    opt no matchmaker
    C->>S: HTTP GET /servers
    S-->>C: ["server-123", "server-456"]
    end
    C->>S: WebSocket connect
    C->>S: register { role: "client", id: "client-abc" }
    S-->>C: register_ok

    Note over C,G: Phase 3: WebRTC Connection (Client initiates)
    C->>S: offer { target_id: "server-123", sdp: "..." }
    Note over S: Validates: client→server allowed
    S->>G: offer { sender_id: "client-abc", sdp: "..." }
    G->>S: answer { target_id: "client-abc", sdp: "..." }
    S->>C: answer { sender_id: "server-123", sdp: "..." }
    
    Note over C,G: Phase 4: Direct Data Channel
    G->>C: Negotiate WebRTC Data Channel(s)
    G<<-->>C: Data
```

### Access Control Rules

```mermaid
graph LR
    subgraph Allowed Connections
        direction LR
        A[Client] -->|offer| B[Server]
        B -->|answer| A
    end

    subgraph Blocked Connections
        direction LR
        C[Client] --x |blocked| D[Client]
    end
```

## Signalling Server Components

### PeerManager

Manages all connected peers and routing:

```go
type PeerInfo struct {
    conn  *websocket.Conn
    role  PeerRole  // RoleClient or RoleServer
}

type PeerManager struct {
    peers  map[string]*PeerInfo  // id → peer info
    servers []string             // list of server IDs
}
```

### HTTP Endpoints

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/health` | GET | Health check |
| `/stats` | GET | Connections count |
| `/servers` | GET | Returns JSON array of server IDs |
| `/connect` | GET, WS | WebSocket signalling endpoint |

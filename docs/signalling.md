# Signalling Server Architecture

## Overview

The signalling server is a Go-based WebSocket relay that facilitates WebRTC connections between game clients and game servers. It uses a role-based registration system to control which peers can connect to each other.

**Key properties:**
- Game servers are publicly discoverable via HTTP `/servers` endpoint
- Clients are hidden (never listed)
- Client-to-client connections are blocked
- Servers act as the offeror, but are triggered by `join` signals from the client
- WebRTC data channels provide low-latency transport after signalling

## Architecture Diagram

```mermaid
graph TD
    subgraph Server
        GS[Game Server<br/>PeerID: random]
    end

    subgraph Client
        C[Game Client<br/>PeerID: random]
    end

    subgraph Signalling
        S[Signalling Server<br/>WebSocket + HTTP]
        SR[(Server Registry)]
    end

    GS -->|WebSocket /connect/...| S
    C -->|WebSocket /connect/...| S
    C -->|HTTP GET /servers| S
    S --> SR
    GS -.->|WebRTC: Server offers| C
    C -.->|WebRTC: Client answers| GS
```

## Registration Protocol

### Message Format

All messages are JSON-encoded. The `id` field is used for both sender identification (in outgoing messages from signalling server) and target identification (in incoming messages to signalling server).

**Registration (first message after WebSocket connect):**
```json
{
  "type": "register",
  "id": "unique-peer-id",
  "role": "client" | "server",
  "name": "server-name",         // optional, server only
  "max_players": 32,             // optional, server only
  "game_mode": "default"         // optional, server only
}
```

**Join (client requests to join a server):**
```json
{
  "type": "join",
  "id": "server-peer-id",
  "role": "client"
}
```

**Offer/Answer (SDP exchange):**
```json
{
  "type": "offer" | "answer",
  "id": "target-peer-id",
  "description": "sdp-string"
}
```

**Candidate (ICE candidate):**
```json
{
  "type": "candidate",
  "id": "target-peer-id",
  "candidate": "ice-candidate-string",
  "mid": "media-stream-id"
}
```

### Connection Flow

```mermaid
sequenceDiagram
    participant C as Client
    participant S as Signalling Server
    participant G as Game Server

    Note over C,G: Phase 1: Server Registration
    G->>S: WebSocket connect to /connect/<server-id>
    G->>S: register { id: "<server-id>", role: "server", name: "MyServer" }
    Note right of S: Server added to /servers list

    Note over C,G: Phase 2: Client Registration
    C->>S: WebSocket connect to /connect/<client-id>
    C->>S: register { id: "<client-id>", role: "client" }

    Note over C,G: Phase 3: Client Joins Server
    C->>S: join { id: "<server-id>", role: "client" }
    S->>G: join { id: "<client-id>" }
    Note over G: Server creates PeerConnection for client

    Note over C,G: Phase 4: WebRTC Negotiation
    G->>S: offer { id: "<client-id>", description: "<sdp>" }
    S->>C: offer { id: "<server-id>", description: "<sdp>" }
    Note over C: Client creates PeerConnection on offer
    C->>S: answer { id: "<server-id>", description: "<sdp>" }
    S->>G: answer { id: "<client-id>", description: "<sdp>" }

    Note over C,G: Phase 5: ICE Candidate Exchange
    loop For each local ICE candidate
        G->>S: candidate { id: "<client-id>", candidate: "...", mid: "..." }
        S->>C: candidate { id: "<server-id>", candidate: "...", mid: "..." }
        C->>S: candidate { id: "<server-id>", candidate: "...", mid: "..." }
        S->>G: candidate { id: "<client-id>", candidate: "...", mid: "..." }
    end

    Note over C,G: Phase 6: Transport
    G<<-->>C: Binary data over WebRTC DataChannels
```

### Access Control Rules

```mermaid
graph LR
    subgraph Allowed Connections
        direction LR
        A[Server] <-->|allowed| B[Client]
        C[Client] x--x |blocked| B
    end
```

**Validation logic (in `handleConnection`):**
- Client-to-client offers are rejected
- Client-to-server offers are allowed
- Server-to-client offers are allowed (though servers don't initiate in current implementation)
- All peers must be registered before sending (except `register` itself)

## Signalling Server Components

### PeerManager

Manages all connected peers and routing:

```go
type PeerInfo struct {
    conn  *websocket.Conn
    role  PeerRole  // RoleClient or RoleServer
}

type PeerManager struct {
    peers   map[string]*PeerInfo  // id → peer info
    servers []string              // list of server IDs
    maxConns int                  // connection limit (0 = unlimited)
}
```

**Key methods:**
- `Register(id, conn, role)` - Registers a peer, returns error if ID exists or max connections reached
- `Unregister(id)` - Removes peer and closes connection
- `Get(id)` - Retrieves peer info by ID
- `GetRole(id)` - Returns peer's role (client/server)
- `GetServerList()` - Returns copy of all server IDs (for `/servers` endpoint)

### Message Routing

The signalling server rewrites the `id` field when forwarding messages:
- **Incoming**: `id` = target peer ID
- **Outgoing**: `id` = sender peer ID

This allows receivers to know who sent the message without an explicit `sender_id` field.

### HTTP Endpoints

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/health` | GET | Health check |
| `/stats` | GET | Connections count (JSON: `{"connections": N}`) |
| `/servers` | GET | Returns JSON array of registered server IDs |
| `/connect/{peer-id}` | GET, WS | WebSocket signalling endpoint (peer-id in URL path) |

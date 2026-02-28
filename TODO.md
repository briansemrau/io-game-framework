# Build System TODO

## Context
- **Goal**: One script per target, no documentation needed
- **Production**: Docker for all server components
- **Development**: Native builds (Windows MSVC, Linux, WASM)
- **WASM**: Native build only (no containerization)

---

## Action Items

### Phase 1: Docker Build Scripts (Production)

- [ ] **Create `scripts/build-docker-server.sh`**
  - Build game server Docker image
  - Accept `release|debug` argument (default: release)
  - Use `--build-arg BUILD_TYPE` to pass to Dockerfile

- [ ] **Update `scripts/build-signalling.sh`**
  - Already exists, builds Docker image ✓
  - Consider adding release/debug option for consistency

- [ ] **Create `scripts/build-docker-all.sh`**
  - Build all Docker images (server + signalling)
  - Convenience wrapper for CI/CD

### Phase 2: Dockerfile Updates

- [ ] **Update `Dockerfile.game-server`**
  - Add `ARG BUILD_TYPE=release`
  - Use build arg for cmake build type and output path
  - Support both `Debug` and `Release` configurations

### Phase 3: Code Changes (Already Done)

- [x] **`src/common/network_server.cpp`** - Read `SIGNALING_URL` env var ✓
- [x] **`Dockerfile.game-server`** - Set `SIGNALING_URL` default ✓
- [x] **`docker-compose.yml`** - Local testing configuration ✓

---

## Current State

| Service | Native Script | Dockerfile | Docker Build Script |
|---------|--------------|------------|---------------------|
| Signalling | N/A | ✓ | ✓ `build-signalling.sh` |
| Game Server | ✓ `build-linux-server.sh` | ✓ `Dockerfile.game-server` | ← TODO |
| Windows Server | ✓ (existing) | N/A | N/A |
| Linux Client | ✓ `build-linux-client.sh` | N/A | N/A |
| Windows Client | ✓ (existing) | N/A | N/A |
| WASM Client | ✓ `build-wasm-client.sh` | N/A | N/A |

---

## Notes

- **Docker debug builds**: For occasional staging debugging, not daily dev
- **Native builds**: Primary development workflow (fast iteration)
- **Docker builds**: Production deployment and CI/CD
- **Windows scripts**: Already exist, no changes needed
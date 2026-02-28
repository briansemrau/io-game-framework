# Monorepo Restructure Plan

## Overview

This document outlines a reorganization plan for the Spite Project monorepo as it grows beyond the core C++ game server/client to include additional services (signalling server, future infrastructure).

**Goal**: Maintain a clean, intuitive structure that scales with the project while respecting tooling expectations (CMake, clangd, etc.).

---

## Current State

```
spite_project/
  .clang-format
  .clangd
  CMakeLists.txt
  CMakePresets.json
  docker-compose.yml
  signalling-server/    # Go signalling server
  src/                  # C++ client/server
    client/
    common/
    server/
    thirdparty/
  scripts/
  infra/
  notes/
  *.md
```

**Issues**:
- Mixed root-level: C++ `src/`, Go `signalling-server/`, infra, scripts, docs
- Unclear where new services should live
- Dockerfile locations not standardized
- Documentation scattered across root and `notes/`

---

## Target Structure

```
spite_project/
  # Root-level tooling (required by build tools)
  .clang-format
  .clangd
  .gitignore
  CMakeLists.txt
  CMakePresets.json
  docker-compose.yml
  README.md
  
  # Core game (C++)
  game/                    # Renamed from src/
    client/
      client_instance.cpp/h
      network_client.cpp/h
      renderer.cpp/h
      debug_draw.cpp/h
      client_main.cpp
      resources/
      Dockerfile           # Client-specific Dockerfile
  
    common/
      game.cpp/h
      car.cpp/h
      network_common.h
      network_server.cpp/h
      server_instance.cpp/h
      state_event.h
      state_event_manager.h
      physics_settings.h
  
    server/
      server_main.cpp
      Dockerfile           # Server-specific Dockerfile
  
    thirdparty/
      box2d/
      raylib/
      entt/
      libdatachannel/
      datachannel-wasm/
      zpp_bits/
      plog/
      json/
  
  # Services (language-agnostic grouping)
  signalling-server/       # Go signalling server (stays at root level)
    main.go
    go.mod
    go.sum
    Dockerfile             # Signalling-specific Dockerfile
  
  # Infrastructure as code
  infra/
    k8s/                   # Future: GameServer CRDs, Fleet configs (Agones)
    redis/
      docker-compose.override.yml
      config.conf
  
  # Build & deployment scripts
  scripts/
    build.sh
    build-linux-client.sh
    build-linux-server.sh
    build-wasm-client.sh
    build-windows-msvc-client.ps1
    build-windows-msvc-server.ps1
    build-signalling.sh
  
  # Documentation
  docs/
    architecture/
      network.md           # From notes/network_architecture.md
      scaling.md           # From notes/scaling_architecture.md
    AGENTS.md              # From root (or keep in root for visibility)
    PERSONAS.md            # From root
    TODO.md                # From root
    restructure-plan.md    # This file
```

---

## Key Design Decisions

### 1. Rename `src/` to `game/`

**Rationale**: 
- More semantic than generic `src/`
- Clarifies this is the game client/server (not just "source code")
- Leaves room for other language-specific dirs if needed (`go/`, `web/`, etc.)

**Trade-off**: Requires updating all CMake paths, but one-time cost.

### 2. Keep `signalling-server/` at root level

**Rationale**:
- It's a distinct service, not part of the game
- Language-agnostic naming (not `go/signalling-server/`)
- Easy to find and work with independently

**Alternative considered**: `services/signalling-server/` but adds unnecessary nesting.

### 3. Per-service Dockerfiles

**Rationale**:
- Follow common convention: Dockerfile lives next to source
- Easy to find: `game/client/Dockerfile`, `signalling-server/Dockerfile`
- Supports service independence

**Trade-off**: Slightly more files at root, but clarity wins.

### 4. Documentation in `docs/`

**Rationale**:
- Consolidates scattered `.md` files
- `notes/` → `docs/architecture/` for better organization
- Keeps `AGENTS.md` and `README.md` at root for visibility (optional)

---

## Migration Plan

### Phase 1: Minimal Disruption (Recommended First Step)

1. **Rename `src/` to `game/`**
   ```bash
   mv src game
   ```

2. **Update `CMakeLists.txt`**
   - Change all `src/` references to `game/`
   - Examples:
     ```cmake
     # Before
     file(GLOB_RECURSE COMMON_SOURCES CONFIGURE_DEPENDS src/common/*.cpp)
     # After
     file(GLOB_RECURSE COMMON_SOURCES CONFIGURE_DEPENDS game/common/*.cpp)
     ```

3. **Move documentation**
   ```bash
   mkdir -p docs/architecture
   mv notes/*.md docs/architecture/
   mv PERSONAS.md TODO.md docs/  # Optional: keep AGENTS.md at root
   ```

4. **Add Dockerfiles** (when created)
   - `game/client/Dockerfile`
   - `game/server/Dockerfile`
   - `signalling-server/Dockerfile` (if not exists)

### Phase 2: Cleanup (Optional, Later)

5. **Consolidate build scripts**
   - Consider using `CMakePresets.json` more extensively
   - Reduce platform-specific script duplication

6. **Add `infra/redis/`**
   - When implementing multi-server architecture
   - Redis config, docker-compose overrides

7. **Add `infra/k8s/`**
   - When migrating to Agones/Kubernetes
   - GameServer CRDs, Fleet configs

---

## Benefits

✅ **Semantic clarity**: `game/` vs `signalling-server/` shows purpose at a glance  
✅ **Tool compatibility**: CMake, clangd, git stay happy with root-level config files  
✅ **Per-service Dockerfiles**: Follow convention, easy to find  
✅ **Future-ready**: Clear places for new services, infra, docs  
✅ **Language-agnostic**: Not tied to C++ or Go specifically  
✅ **Scalable**: Easy to add more services (matchmaking, leaderboard, web dashboard)  

---

## What Stays the Same

- **C++ code structure**: No changes to `game/client/`, `game/common/`, `game/server/` internals
- **Thirdparty structure**: `game/thirdparty/` remains unchanged
- **Build scripts**: `scripts/` stays as-is (can be cleaned up later)
- **Root tooling**: `.clang-format`, `.clangd`, `CMakeLists.txt` stay at root

---

## What Changes

- **Directory name**: `src/` → `game/`
- **CMake paths**: All `src/` references updated to `game/`
- **Documentation**: Scattered `.md` files consolidated into `docs/`
- **Dockerfiles**: Added per-service (when created)

---

## Rollback Plan

If needed, rollback is trivial:

```bash
# Restore original structure
mv game src
git checkout CMakeLists.txt
mv docs/architecture/*.md notes/
mv docs/*.md .  # Move back to root
rmdir docs
```

---

## Next Steps

1. ✅ Review and approve this plan
2. ⏳ Execute Phase 1 migration
3. ⏳ Verify builds work after changes
4. ⏳ Add Dockerfiles as services are developed
5. ⏳ Consider Phase 2 cleanup when convenient

---

## Questions & Decisions Made

| Question | Decision | Rationale |
|----------|----------|-----------|
| Where should signalling server live? | Root level (`signalling-server/`) | It's a distinct service; language-agnostic naming |
| How should Dockerfiles be organized? | Per-service | Follows convention, easy to find |
| Should C++ move to `cpp/` subdir? | Rename to `game/` | Semantic clarity, leaves room for other languages |
| Where should docs go? | `docs/` directory | Consolidates scattered files |

---

*Created: 2026-02-27*  
*Status: Proposal - Awaiting Approval*
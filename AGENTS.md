# AGENTS.md - Development Guidelines for io-game-framework

## Overview

This is a C++23 server-authoritative multiplayer game demo.

The goal is to create a clean prototype demonstrating pristine design. The architecture is designed to be **extensible and reusable** for future projects.

**Design philosophy**: Simple, clean code first. Abstraction only when needed for extensibility. Build a solid foundation that can grow.

## Deployable Components

| Component | Location | Description |
|-----------|----------|-------------|
| **client** | `game/src/client/` | Desktop/WASM executable with graphics (raylib), input handling, and predictive simulation |
| **server** | `game/src/server/` | Headless simulation server |
| **signalling-server** | `signalling-server/` | Go WebSocket relay for WebRTC peer handshake |
| **Dockerfiles** | `game/Dockerfile.game-server`, `signalling-server/Dockerfile.signalling` | Container images for server deployment |

## Project Structure

```
game/
  src/
    client/     - Client executable code (graphics, input)
    common/     - Shared code (game state, systems, networking)
    server/     - Server executable code (no graphics or input)
  thirdparty/   - Third party dependencies (git submodules, do not edit)
  Dockerfile.game-server
scripts/        - Build scripts for various platforms
docs/           - Architecture documentation
infra/          - Infrastructure as code (docker-compose, etc.)
signalling-server/  - Go-based WebRTC signalling server
```

## Build System

- **CMake 3.25+** with **Ninja Multi-Config** generator
- **C++23** standard (strict, no extensions)
- Uses `CMakePresets.json` for platform-specific configurations

### Platforms

| Platform | Client | Server | Preset |
|----------|--------|--------|--------|
| **Windows (MSVC)** | ✅ | ✅ | `windows-client-debug`, `windows-server-debug` |
| **Linux (Clang)** | ✅ | ✅ | `linux-client-debug`, `linux-server-debug` |
| **WASM (Emscripten)** | ✅ | ❌ | `wasm-client-debug` |

### Build Commands

```bash
# Configure
cmake --preset <preset-name>

# Build
cmake --build --preset <preset-name>
```

### Scripts

- `scripts/build.sh` - Linux server + WASM client
- `scripts/build-linux-{client,server}.sh` - Linux native builds
- `scripts/build-windows-msvc-{client,server}.ps1` - Windows MSVC builds
- `scripts/build-wasm-client.sh` - WebAssembly client
- `scripts/build-docker-{server,all}.sh` - Docker images

## Dependencies

| Dependency | Type | Purpose |
|------------|------|---------|
| **raylib** | Compiled | Client graphics/window management |
| **box2d** | Compiled | Physics simulation (C API) |
| **entt** | Header-only | Entity-component system |
| **libdatachannel** | Compiled | WebRTC (native) |
| **datachannel-wasm** | Header-only | WebRTC (WASM) |
| **zpp_bits** | Header-only | Binary serialization |
| **nlohmann/json** | Header-only | JSON parsing |
| **plog** | Header-only | Logging |

A third party library for compression will be required in the future.

## Architectural Principles

- **Common is the heart** - If something belongs in both client and server, it goes in common. The client is an extension, not a shortcut.
- **Client handles presentation** - Renderer wraps game state, input generates events that manipulate state, client maintains local state for rollback and resimulation. Client does not own or directly control game state.
- **Prefer future-proof over simple** - A slightly more complex approach that enables extensibility beats the simplest solution that paints you into a corner.
- **Intentional architecture** - Explicit is better than implicit. Boundaries between systems should be clear, not accidental.
- **No shortcuts** - Code written today is code you'll maintain forever.

## Code Style

As a baseline, use the Google C++ Style Guide (https://google.github.io/styleguide/cppguide.html)

Our exceptions to the Google style guide include:
- Use the C++23 standard
- Don't worry about formatting (we use clang-format) but naming conventions do matter.

Don't assume that the code you're editing correctly follows the style. This should be identified when asked to code review.

## Testing

There are no automated tests in the main build. This will need to be improved.

**Recommendation**: Use Google Test (gtest). Add to `CMakeLists.txt`:
```cmake
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
FetchContent_MakeAvailable(googletest)
include(GoogleTest)
```

Then create tests in `game/tests/` and add with `add_test()` and `gtest_discover_tests()`.

## Git Workflow

Do not create git commits. You may view status, but the user is in control of the git history.

The only exception to this rule is on "*-freeform" branches. You may never switch branches, but you may commit and revert in a *-freeform branch. The user will typically prompt you to use git in these situations.

## Deployment

### Docker

```bash
# Build all images
bash scripts/build-docker-all.sh [release|debug]

# Run local development
docker-compose -f infra/docker-compose.local-debug.yml up -d
```

### Environment Variables

- `SIGNALING_URL` - WebRTC signalling server URL (default: `localhost:8080`)

## Additional Documentation

- `docs/network_architecture.md` - Detailed network architecture with rollback resimulation design
- `infra/README.md` - Infrastructure notes
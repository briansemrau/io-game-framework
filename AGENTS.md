# AGENTS.md - Development Guidelines for Spite Project

## Overview

This is a C++23 CMake-based game project using [raylib](https://www.raylib.com/) for the client and [box2d](https://box2d.org/) for physics. The project builds two executables: `client` (graphics + input) and `server` (physics simulation).

This is not a throwaway prototype. The architecture is designed to be **extensible and reusable** for future projects. Consider: would this approach work for a different game?

- Server-authoritative networking with rollback and client prediction
- WebRTC or similar transport layers
- Binary serialization (e.g., zpp::bits)

**Design philosophy**: Simple, clean code first. Abstraction only when needed for extensibility. Build a solid foundation that can grow.

## Architectural Principles

- **Common is the heart** - If something belongs in both client and server, it goes in common. The client is an extension, not a shortcut.
- **Client handles presentation** - Renderer wraps game state, input generates events that manipulate state, client maintains local state for rollback and resimulation. Client does not own or directly control game state.
- **Prefer future-proof over simple** - A slightly more complex approach that enables extensibility beats the simplest solution that paints you into a corner.
- **Intentional architecture** - Explicit is better than implicit. Boundaries between systems should be clear, not accidental.
- **No shortcuts** - Code written today is code you'll maintain forever.

### Server-Authoritative Design
- All game logic lives in `common/` (shared between client/server)
- Server owns physics truth; client predicts and interpolates
- Game state is reproducible for rollback
- Network protocol is an abstraction - prepare for WebRTC or similar

### Extensibility
- Design for addition, not modification
- New entity types should not require changes to core architecture
- Consider serialization concerns early (snapshot/restore for rollback)
- Use interfaces/abstractions where multiple implementations may be needed

## Build Commands

Build commands are not available for agents.

### VS Code Tasks
The following tasks are configured in `.vscode/tasks.json`:
- `CMake: Configure` - Configure CMake
- `CMake: Build Client (Debug)` - Build client executable (Debug)
- `CMake: Build Client (Release)` - Build client executable (Release)
- `CMake: Build Server (Debug)` - Build server executable (Debug)
- `CMake: Build Server (Release)` - Build server executable (Release)
- `CMake: Build All (Debug)` - Build all targets (Debug)
- `CMake: Build All (Release)` - Build all targets (Release)

### Running the Applications
```bash
# Run client (from project root)
./build/client/Debug/client.exe   # Debug build
./build/client/Release/client.exe  # Release build

# Run server (from project root)
./build/server/Debug/server.exe   # Debug build
./build/server/Release/server.exe  # Release build
```

## Project Structure
```
src/
  client/     - Client executable code (graphics, input)
  common/     - Shared code (game state, systems, physics)
  server/     - Server executable code (no graphics or input)
thirdparty/
  raylib/     - Graphics library (git submodule)
  box2d/      - Physics library (git submodule)
  entt/       - Entity component system (git submodule)
  libdatachannel/ - WebRTC/networking library (git submodule)
  zpp_bits/   - Binary serialization library (git submodule)
```

## Code Style Guidelines

As a baseline, use the Google C++ Style Guide (https://google.github.io/styleguide/cppguide.html)

Our exceptions to the Google style guide include:
- Use the C++23 standard
- There is no line length limit
- Use 4 spaces for indentation

## Dependencies
- **raylib**: Client-side graphics and window management
- **box2d**: Physics simulation (C API version, `b2WorldId`, `b2BodyId`, etc.)
- **entt**: Entity component system
- **libdatachannel**: WebRTC/networking transport
- **zpp_bits**: Binary serialization for transport

## Testing
There are no automated tests in the main build.

## Git Workflow
Do not create git commits. You may view status, but the user is in control of the git history.
The only exception to this rule is on "*-freeform" branches. You may never switch branches, but you may commit and revert in a *-freeform branch. The user will typically prompt you to use git in these situations.

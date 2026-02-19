# AGENTS.md - Development Guidelines for Spite Project

## Overview

This is a C++23 CMake-based game project using [raylib](https://www.raylib.com/) for the client and [box2d](https://box2d.org/) for physics. The project builds two executables: `client` (graphics + input) and `server` (physics simulation).

This is not a throwaway prototype. The architecture is designed to be **extensible and reusable** for future projects. Consider: would this approach work for a different game?

- Server-authoritative networking with rollback and client prediction
- WebRTC or similar transport layers
- Binary serialization (e.g., zpp::bits)

**Design philosophy**: Simple, clean code first. Abstraction only when needed for extensibility. Build a solid foundation that can grow.

**Architectural principles:**
- **Common is the heart** - If something belongs in both client and server, it goes in common. The client is an extension, not a shortcut.
- **Client handles presentation** - Renderer wraps game state, input generates events that manipulate state, client maintains local state for rollback and resimulation. Client does not own or directly control game state.
- **Prefer future-proof over simple** - A slightly more complex approach that enables extensibility beats the simplest solution that paints you into a corner.
- **Intentional architecture** - Explicit is better than implicit. Boundaries between systems should be clear, not accidental.
- **No shortcuts** - Code written today is code you'll maintain forever.

## Build Commands

### VS Code Tasks
The following tasks are configured in `.vscode/tasks.json`:
- `CMake: Configure` - Configure CMake
- `CMake: Build Client` - Build client executable
- `CMake: Build Server` - Build server executable
- `CMake: Build All` - Build all targets

### Running the Applications
```bash
# Run client (from build directory)
./build/client

# Run server (from build directory)
./build/server
```

## Code Style Guidelines

### General Principles
- C++23 standard is required
- Prefer modern C++ features (concepts, constexpr, std::format, etc.)
- This project aims to have a very clean codebase. Architecture and build configuration should be very maintainable and professional.

### Naming Conventions
- **Types** (classes, structs, enums): PascalCase (e.g., `PhysicsSettings`, `CollisionBits`)
- **Functions**: snake_case (e.g., `init_common`, `step_common`)
- **Variables**: snake_case (e.g., `worldId`, `throttleInput`)
- **Constants**: PascalCase with `constexpr` (e.g., `pixelsPerMeter`)
- **Enum values**: PascalCase (e.g., `StaticBit`, `MoverBit`)

### Formatting
- Use tabs for indentation
- Opening braces on same line (K&R style)
- Max line length: aim for readability, no hard limit

### Header Organization
Order includes as follows:
1. External library headers (e.g., `#include "box2d/box2d.h"`, `#include "raylib.h"`)
2. Standard library headers (e.g., `#include <vector>`, `#include <format>`)
3. Project headers (e.g., `#include "physics_settings.h"`)

Use include guards (not `#pragma once`):
```cpp
#ifndef FILE_NAME_H
#define FILE_NAME_H
// content
#endif // FILE_NAME_H
```

### Types
- Use explicit integer types from `<cstdint>` for interop (e.g., `uint32_t`, `uint64_t`)
- Use `enum class` for new enums, or C-style enums with explicit underlying type for bitflags:
  ```cpp
  enum CollisionBits : uint64_t
  {
      StaticBit = 0x0001,
      DynamicBit = 0x0004,
  };
  ```
- Prefer `constexpr` over `#define` for compile-time constants

### Functions
- Prefer `[[nodiscard]]` attribute for functions that must not be ignored
- Use `void` explicitly for functions taking no parameters
- Pass large objects by const reference: `const MyType&`
- Return by value unless moving or forwarding

### Error Handling
- Use assertions (`assert.h`) for debug-only invariant checking
- For recoverable errors, prefer returning error codes or using `std::expected` (C++23)
- Avoid exceptions unless absolutely necessary (this codebase doesn't use them)

### Lambda Expressions
- Capture by reference `[&]` when appropriate for performance
- Specify return type if ambiguous or to aid readability
- Use for callbacks and STL algorithm customization

### Dependencies
- **raylib**: Client-side graphics and window management
- **box2d**: Physics simulation (C API version, `b2WorldId`, `b2BodyId`, etc.)

### Project Structure
```
src/
  client/     - Client executable code (graphics, input)
  common/     - Shared code (game state, systems, physics)
  server/     - Server executable code (no graphics or input)
thirdparty/
  raylib/     - Graphics library (git submodule)
  box2d/      - Physics library (git submodule)
```

### Architecture Guidelines

#### Server-Authoritative Design
- All game logic lives in `common/` (shared between client/server)
- Server owns physics truth; client predicts and interpolates
- Game state is reproducible for rollback
- Network protocol is an abstraction - prepare for WebRTC or similar

#### Extensibility
- Design for addition, not modification
- New entity types should not require changes to core architecture
- Consider serialization concerns early (snapshot/restore for rollback)
- Use interfaces/abstractions where multiple implementations may be needed

### Testing
- There are no automated tests in the main build

### Git Workflow
Do not create git commits. You may view status, but the user is in control of the git history.

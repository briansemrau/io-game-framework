# AGENTS.md - Development Guidelines for Spite Project

## Overview

This is a C++23 server-authoritative multiplayer game demo using [raylib](https://www.raylib.com/) for the client and [box2d](https://box2d.org/) for physics. The project builds two executables: `client` (graphics + input) and `server` (physics simulation). Communication is done using WebRTC Data Channels, compressed using (dependency trade study incomplete), and serialized using zpp::bits.

The goal is to create a clean prototype demonstrating pristine design. The architecture is designed to be **extensible and reusable** for future projects.

**Design philosophy**: Simple, clean code first. Abstraction only when needed for extensibility. Build a solid foundation that can grow.

## Architectural Principles

- **Common is the heart** - If something belongs in both client and server, it goes in common. The client is an extension, not a shortcut.
- **Client handles presentation** - Renderer wraps game state, input generates events that manipulate state, client maintains local state for rollback and resimulation. Client does not own or directly control game state.
- **Prefer future-proof over simple** - A slightly more complex approach that enables extensibility beats the simplest solution that paints you into a corner.
- **Intentional architecture** - Explicit is better than implicit. Boundaries between systems should be clear, not accidental.
- **No shortcuts** - Code written today is code you'll maintain forever.

## Project Structure
```
src/
  client/     - Client executable code (graphics, input)
  common/     - Shared code (game state, systems, physics)
  server/     - Server executable code (no graphics or input)
thirdparty/   - Third party dependencies (do not edit)
scripts/      - All project build scripts
```

## Code Style Guidelines

As a baseline, use the Google C++ Style Guide (https://google.github.io/styleguide/cppguide.html)

Our exceptions to the Google style guide include:
- Use the C++23 standard
- Don't worry about formatting (we use clang-format) but naming conventions do matter.

Don't assume that the code you're editing correctly follows the style. This should be identified when asked to code review.

## Dependencies
- **raylib**: Client-side graphics and window management
- **box2d**: Physics simulation (C API version, `b2WorldId`, `b2BodyId`, etc.)
- **entt**: Entity component system
- **libdatachannel** and **datachannel-wasm**: WebRTC/networking transport
- **zpp_bits**: Binary serialization for transport

## Testing
There are no automated tests in the main build. This will need to be improved. Considering using Google Test.

## Git Workflow
Do not create git commits. You may view status, but the user is in control of the git history.
The only exception to this rule is on "*-freeform" branches. You may never switch branches, but you may commit and revert in a *-freeform branch. The user will typically prompt you to use git in these situations.

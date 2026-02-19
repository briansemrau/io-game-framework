# TODO.md - Cleanup Tasks

## Code Style Fixes

### Header Organization
- [x] Fix include order in `common_main.cpp` - stdlib before project headers
- [x] Fix include order in `car.cpp` - stdlib before project headers
- [x] Fix include order in `client_main.cpp` - stdlib before project headers

### Type Fixes
- [x] Replace `enum CollisionBits` with `enum class` in `physics_settings.h`
- [x] Rename `#ifndef PHYSICS_H` to `#ifndef PHYSICS_SETTINGS_H` in `physics_settings.h`
- [x] Replace `<assert.h>` with `<cassert>` in `client_main.cpp`

### Magic Numbers
- [x] Extract `1.0f / 60.0f` delta time to named constant in `common_main.cpp`
- [x] Replace `3.14159f` with `PI` from `<cmath>` in `common_main.cpp`
- [x] Move hardcoded arena dimensions to constants in `common_main.cpp`

### Duplicate Definitions
- [x] Consolidate `CollisionBits` and `CarCollisionBits` to single authoritative source
- [x] Update all collision bit references to use consolidated enum

### Dead Code Removal
- [x] Remove unused `src/CMakeLists.txt` subdirectory
- [x] Remove unused `updateAICars` forward declaration or implement it properly (common_main.cpp:14)

### Toggleable Debug Draw Integration
- [x] Make debug draw toggleable in client (press 'G' key to enable/disable)
- [x] Integrate `b2RaylibDebugDraw()` into client render loop when enabled

## Minor Improvements

- [x] Add graceful exit condition to server (e.g., signal handling or stdin check)
- [x] Add `[[nodiscard]]` to functions that return important values where appropriate

# Future Suggestions

## Code Architecture
- Move arena dimensions to a centralized config/state structure shared between client and server
- Consider extracting rendering constants (zoom, colors, UI layout) to a separate config file
- Add a proper game state object to encapsulate simulation state for cleaner serialization

## Debugging & Development
- The debug draw currently renders at a fixed scale (`pixelsPerMeter`) which differs from the client's zoom - consider aligning these or making the debug draw camera-aware
- Add more detailed logging or a debug console for development
- Consider adding unit tests for physics calculations and game logic

## Performance
- Profile the client rendering to identify bottlenecks
- Consider object pooling for frequently created/destroyed entities (trail points, particles)

## Networking Preparation
- Design the serialization format for game state snapshots (mentioned in AGENTS.md as zpp::bits)
- Define the network protocol message types for client-server communication
- Consider adding a local "replay" mode that records and plays back game states for rollback testing

# TODO.md - Cleanup Tasks

## Code Style Fixes

### Header Organization
- [ ] Fix include order in `common_main.cpp` - stdlib before project headers
- [ ] Fix include order in `car.cpp` - stdlib before project headers
- [ ] Fix include order in `client_main.cpp` - stdlib before project headers

### Type Fixes
- [ ] Replace `enum CollisionBits` with `enum class` in `physics_settings.h`
- [ ] Rename `#ifndef PHYSICS_H` to `#ifndef PHYSICS_SETTINGS_H` in `physics_settings.h`
- [ ] Replace `<assert.h>` with `<cassert>` in `client_main.cpp`

### Magic Numbers
- [ ] Extract `1.0f / 60.0f` delta time to named constant in `common_main.cpp`
- [ ] Replace `3.14159f` with `PI` from `<cmath>` in `common_main.cpp`
- [ ] Move hardcoded arena dimensions to constants in `common_main.cpp`

### Duplicate Definitions
- [ ] Consolidate `CollisionBits` and `CarCollisionBits` to single authoritative source
- [ ] Update all collision bit references to use consolidated enum

### Dead Code Removal
- [ ] Remove unused `src/CMakeLists.txt` subdirectory
- [ ] Remove unused `updateAICars` forward declaration or implement it properly (common_main.cpp:14)

### Toggleable Debug Draw Integration
- [ ] Make debug draw toggleable in client (e.g., press 'D' key to enable/disable)
- [ ] Integrate `b2RaylibDebugDraw()` into client render loop when enabled

## Minor Improvements

- [ ] Add graceful exit condition to server (e.g., signal handling or stdin check)
- [ ] Add `[[nodiscard]]` to functions that return important values where appropriate

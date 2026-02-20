# TODO - Architectural Improvements

This document outlines the tasks needed to address the issues identified in `ARCHITECTURAL_REVIEW.md`.

---

## Phase 1: Critical Fixes (Immediate)

### 1.2 Encapsulate Physics Engine Types
**Files**: `src/common/car.h`, `src/common/car.cpp`

- [ ] Change `bodyId` from public to private member
- [ ] Add accessor method: `b2BodyId getBodyId() const { return bodyId; }`
- [ ] Update all direct accesses in `renderer.cpp` to use existing accessor methods (`getPosition()`, `getVelocity()`, `getAngle()`)

### 1.3 Move pixelsPerMeter Out of Common
**Files**: `src/common/physics_settings.h`, `src/client/debug_draw.cpp`, `src/client/renderer.cpp`

- [ ] Remove `static constexpr float pixelsPerMeter = 64.0f;` from `physics_settings.h`
- [ ] Add `pixelsPerMeter` to the renderer code

### 1.4 Fix Hardcoded Arena Dimensions
**Files**: `src/client/renderer.cpp`

- [ ] Replace hardcoded `40.0f` with `GameState::ArenaWidth`
- [ ] Replace hardcoded `30.0f` with `GameState::ArenaHeight`
- [ ] Lines 26-27: `arenaW`, `arenaH`
- [ ] Lines 38-46: Grid rendering bounds

---

## Phase 2: Code Cleanup (Short-term)

### 2.1 Remove Empty Input Module
**Files**: `src/client/input.h`

- [ ] Implement `input.cpp` with proper input handling

### 2.2 Remove Duplicate carSetInput
**Files**: `src/common/car.h`, `src/common/car.cpp`

- [ ] Delete free function `carSetInput` (lines 39 in .h, 95-97 in .cpp)
- [ ] Update `client_main.cpp` line 88 to use `playerCar.setInput()` instead

### 2.3 Commented Code
**Files**: `src/client/renderer.cpp`

- [ ] Address commented debug draw code on lines 121-122:
  ```cpp
  // b2World_DrawWorld(getWorldId(), &debugDraw); // TODO what is the right function?
  ```

### 2.4 Standardize Naming Conventions
**Files**: `src/common/game_state.h`, `src/common/game_state.cpp`

- [ ] Review all constants, ensure `PascalCase` for all
- [ ] Ensure file-scope constants match class constants

---

## Phase 3: Architecture Improvements (Medium-term)

### 3.1 Extract AI System
**Files**: `src/common/game_state.h`, `src/common/game_state.cpp`

- [ ] Create new files: `src/common/ai_system.h`, `src/common/ai_system.cpp`
- [ ] Move `updateAICars()` logic to new AI system
- [ ] Update GameState to own an AI system instance
- [ ] Consider configurable AI behavior through parameters

### 3.4 Extract Magic Numbers to Constants
**Files**: `src/common/car.h`, `src/common/game_state.cpp`

- [ ] Car physics constants (car.h):
  - `acceleration = 20.0f` → named constant
  - `turnSpeed = 3.5f` → named constant
  - `friction = 0.98f` → named constant
  - `driftFactor = 0.95f` → named constant
- [ ] AI timing constants (game_state.cpp):
  - `0.5f`, `0.3f`, `3.0f` → named constants

---

## Phase 4: Future Improvements (Long-term)

### 4.1 Consider ECS Architecture
**Files**: Multiple

- [ ] Refactor Car, Obstacle to be entt components
- [ ] This enables easier addition of new entity types

### 4.4 Complete Input System
**Files**: `src/client/input.h`, new `input.cpp`

- [ ] Implement proper input handling module

### 4.5 Improve CMakeLists.txt
**Files**: `CMakeLists.txt`

- [ ] Replace GLOB_RECURSE with explicit source lists
- [ ] Or document that CMake must be re-run for new files

---

## Verification Checklist

After completing each phase, verify:

- [ ] Coding style is followed per AGENTS.md

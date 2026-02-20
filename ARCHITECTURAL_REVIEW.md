# Architectural Review - Spite Project

This document reviews the codebase with focus on extensibility, maintainability, and code clarity. Issues are categorized by severity and location.

---

## Critical Issues

### 1. Duplicate Time Constants (game_state.h:11-12, 22-23)
**Severity**: High  
**Category**: Code Clarity / DRY Violation

The same constants are defined twice:
- File-scope: `static constexpr uint32_t fixed_timesteps_per_second = 60;`
- Class-scope: `static constexpr uint32_t FixedTimestepsPerSecond = 60;`

This creates confusion about which to use and violates DRY. Additionally, `fixed_timestep_duration` at file scope (line 12) differs from `FixedTimestepDuration` in the class (line 23).

**Recommendation**: Remove file-scope constants, use class constants consistently.

### 2. Public Physics Engine Types (car.h:23)
**Severity**: High  
**Category**: Encapsulation / Extensibility

```cpp
b2BodyId bodyId = {};  // Public, direct box2d type
```

Exposing `b2BodyId` publicly leaks the physics engine implementation into all code that uses `Car`. This creates tight coupling to box2d throughout the codebase.

**Recommendation**: Use a type-erased or wrapper ID, or make the bodyId private with accessor methods.

### 3. Direct Access to Internal State (renderer.cpp:68, 108, 112)
**Severity**: High  
**Category**: Encapsulation / Architecture

The renderer directly accesses `car.bodyId`:
```cpp
b2Vec2 aiCarPos = b2Body_GetTransform(aiCar.bodyId).p;
```

This bypasses Car's interface and creates tight coupling between renderer and physics internals.

**Recommendation**: Use Car's accessor methods (`getPosition()`, `getVelocity()`, etc.)

### 4. Common Module Depends on Rendering Constants (physics_settings.h:6)
**Severity**: High  
**Category**: Module Boundaries

```cpp
static constexpr float pixelsPerMeter = 64.0f;
```

This constant belongs to the presentation layer (client), not common. The common module should be independent of rendering concerns.

**Recommendation**: Move `pixelsPerMeter` to client code only.

### 5. Hardcoded Arena Dimensions Duplicated (renderer.cpp:26-27, 38-46)
**Severity**: High  
**Category**: DRY Violation

The renderer hardcodes arena dimensions:
```cpp
float arenaW = 40.0f * zoom;  // Should use GameState::ArenaWidth
float arenaH = 30.0f * zoom;  // Should use GameState::ArenaHeight
```

**Recommendation**: Use `GameState::ArenaWidth` and `GameState::ArenaHeight` directly.

---

## Moderate Issues

### 6. Empty/Unused Input Module (input.h)
**Severity**: Medium  
**Category**: Code Clarity

The input.h header exists but is essentially empty:
```cpp
void handleInput(const GameState&, const RenderState&);
```
There's no implementation file, and all input handling is done inline in client_main.cpp.

**Recommendation**: Either implement a proper input system or remove the placeholder.

### 7. Duplicate carSetInput Function (car.h:39, car.cpp:95-97)
**Severity**: Medium  
**Category**: Code Clarity

Both a method and free function do the same thing:
```cpp
void Car::setInput(float throttle, float turn, bool handbrake);
void carSetInput(Car& car, float throttle, float turn, bool handbrake);
```

**Recommendation**: Remove the redundant free function.

### 8. No Entity Component System (ECS) Despite Dependency
**Severity**: Medium  
**Category**: Architecture / Extensibility

The project has `entt` as a dependency but doesn't use it. Entities (cars, obstacles) are managed as raw C++ objects in vectors:
```cpp
Car playerCar;
std::vector<Car> aiCars;
std::vector<Obstacle> obstacles;
```

This limits extensibility for adding new entity types.

**Recommendation**: Consider using entt for entity management to improve extensibility.

### 9. AI Logic Hardcoded in GameState (game_state.cpp:132-169)
**Severity**: Medium  
**Category**: Single Responsibility

AI behavior is embedded directly in GameState::updateAICars(). This violates single responsibility and makes AI logic difficult to modify or extend.

**Recommendation**: Extract AI into a separate system/component.

### 10. Unused/Commented Code (renderer.cpp:121-122)
**Severity**: Medium  
**Category**: Code Clarity
```cpp
// b2World_DrawWorld(getWorldId(), &debugDraw); // TODO what is the right function?
```

Dead code should be removed, not commented.

### 11. No Serialization Despite zpp_bits Dependency
**Severity**: Medium  
**Category**: Architecture

zpp_bits is included as a dependency for binary serialization, but no serialization code exists. The TODO comments mention networking but there's no snapshot/rollback mechanism.

**Recommendation**: Implement serialization for game state snapshots to support rollback and networking.

### 12. Inconsistent Naming Conventions (game_state.h)
**Severity**: Medium  
**Category**: Code Style

Mixed naming styles for constants:
- File-scope: `snake_case` (fixed_timesteps_per_second)
- Class-scope: `PascalCase` (FixedTimestepsPerSecond)

**Recommendation**: Standardize on PascalCase for all constants per AGENTS.md.

### 13. Missing noexcept Specifications
**Severity**: Medium  
**Category**: Modern C++

Several functions that could benefit from `noexcept` don't have it:
- Getters in Car class
- Accessor methods in GameState

---

## Minor Issues / Code Style

### 14. Magic Numbers Throughout
**Severity**: Low  
**Category**: Maintainability

Numerous hardcoded values without explanation:
- car.h: `acceleration = 20.0f`, `turnSpeed = 3.5f`, `friction = 0.98f`
- game_state.cpp: AI timing `0.5f`, `0.3f`, `3.0f`, positions

**Recommendation**: Extract to named constants or configuration structs.

### 15. (void)deltaTime Suppression (car.cpp:44)
**Severity**: Low  
**Category**: Code Style
```cpp
void Car::update(float deltaTime) {
    (void)deltaTime;  // Workaround for unused parameter
```

Better to either use the parameter or remove it.

### 16. RenderState is a Plain Struct (renderer.h:8-12)
**Severity**: Low  
**Category**: Architecture

RenderState has no implementation file and minimal functionality. Consider whether it's needed or can be integrated elsewhere.

### 17. No Error Handling
**Severity**: Low  
**Category**: Robustness

Most functions return void with no error reporting. No use of std::expected or error codes.

**Recommendation**: Add error handling for critical operations (initialization, physics creation).

### 18. No Logging Infrastructure
**Severity**: Low  
**Category**: Maintainability

Server uses std::cout, client has no logging. Inconsistent debugging experience.

**Recommendation**: Add a simple logging abstraction.

### 19. GameState Tightly Couples Game Logic
**Severity**: Low  
**Category**: Extensibility

GameState contains all game logic (player, AI, obstacles, arena). Adding new entity types requires modifying GameState directly.

**Recommendation**: Consider a more component-based architecture.

---

## Build/Configuration Issues

### 20. CMake Uses GLOB_RECURSE (CMakeLists.txt:50-51, 55)
**Severity**: Low  
**Category**: Build System

```cpp
file(GLOB_RECURSE CLIENT_SOURCES "${CMAKE_SOURCE_DIR}/src/client/*.cpp")
```

GLOB_RECURSE doesn't automatically detect new files without re-running CMake. Better to explicitly list sources or use CMake's --fresh flag.

---

## Summary Checklist

| Issue | Severity | Location |
|-------|----------|----------|
| Duplicate time constants | High | game_state.h:11-12, 22-23 |
| Public b2BodyId | High | car.h:23 |
| Direct bodyId access | High | renderer.cpp:68,108,112 |
| pixelsPerMeter in common | High | physics_settings.h:6 |
| Hardcoded arena dims | High | renderer.cpp |
| Empty input module | Medium | input.h |
| Duplicate carSetInput | Medium | car.h:39, car.cpp:95 |
| No ECS usage | Medium | N/A |
| AI in GameState | Medium | game_state.cpp:132 |
| Dead code | Medium | renderer.cpp:121 |
| No serialization | Medium | N/A |
| Inconsistent naming | Medium | game_state.h |
| Missing noexcept | Medium | Various |
| Magic numbers | Low | Multiple files |
| Unused parameter | Low | car.cpp:44 |
| No error handling | Low | Multiple files |
| No logging | Low | N/A |

---

## Recommendations Priority

1. **Immediate**: Fix duplicate constants, remove public b2BodyId exposure, move pixelsPerMeter out of common
2. **Short-term**: Implement proper input system, remove dead code, standardize naming
3. **Medium-term**: Add serialization (zpp_bits), consider ECS architecture, extract AI system
4. **Long-term**: Complete separation of presentation and game logic, add logging and error handling

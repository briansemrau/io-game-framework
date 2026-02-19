# Architectural Review: Spite Project

**Reviewer:** System Architect  
**Date:** 2026-02-18  
**Scope:** Full codebase review (client, common, server)

---

## Executive Summary

This project claims for extensibility and reuse, with explicit to be built goals around server-authoritative networking, rollback, and client prediction. However, the current implementation has significant gaps between stated architecture and actual code. The foundation has structural issues that will constrain future growth.

---

## 1. Code Style Violations

### 1.1 Header Include Order (AGENTS.md §Header Organization)

**Requirement:** External → Standard Library → Project headers

| File | Violation |
|------|-----------|
| `common_main.cpp:1-5` | Project headers (`common_main.h`, `physics_settings.h`) before stdlib (`<cstddef>`, `<vector>`, `<cmath>`) |
| `car.cpp:1-3` | Project headers before stdlib (`<cmath>`) |
| `client_main.cpp:1-12` | Multiple: stdlib mixed incorrectly, project headers at end |

### 1.2 Include Guard Style (AGENTS.md §Header Organization)

- `physics_settings.h:1-2` uses `#ifndef PHYSICS_H` - should be `PHYSICS_SETTINGS_H`

### 1.3 Enum Style (AGENTS.md §Types)

- `physics_settings.h:10` uses C-style enum instead of `enum class`
- `car.h:25-29` correctly uses `enum class`

### 1.4 Deprecated Headers

- `client_main.cpp:7` uses `<assert.h>` instead of `<cassert>`

### 1.5 Magic Numbers

| Location | Issue |
|----------|-------|
| `common_main.cpp:99` | `1.0f / 60.0f` - should use constant |
| `common_main.cpp:185-186` | `3.14159f` - should use `PI` from `<cmath>` |
| `common_main.cpp:10-11` | Arena dimensions hardcoded as `{-20.0f, -15.0f}`, `{20.0f, 15.0f}` |
| `common_main.cpp:60-65` | Obstacle positions hardcoded inline |

---

## 2. Architectural Issues

### 2.1 No Networking Layer (Critical Gap)

**Stated goal (AGENTS.md):**
> Server-authoritative networking with rollback and client prediction  
> WebRTC or similar transport layers  
> Network protocol is an abstraction - prepare for WebRTC or similar

**Reality:**
- Zero networking code exists
- `server_main.cpp:6-8` is an infinite loop with no network I/O
- No `NetworkClient`, `NetworkServer`, or protocol abstraction
- No `Snapshot` or serialization types

**Impact:** The architecture cannot support multiplayer. Adding networking later will require rewriting core systems.

### 2.2 No Serialization for Rollback (Critical Gap)

**Stated goal (AGENTS.md):**
> Consider serialization concerns early (snapshot/restore for rollback)  
> Game state is reproducible for rollback

**Reality:**
- `Car` struct contains `b2BodyId` - inherently non-serializable (opaque Box2D handle)
- No `serialize()` / `deserialize()` functions
- No snapshot system

**Impact:** Cannot implement rollback or state reconciliation without major refactoring.

### 2.3 Client Directly Controls Game State (Architectural Violation)

**Stated goal (AGENTS.md):**
> Client handles presentation - Renderer wraps game state, input generates events that manipulate state, client maintains local state for rollback and resimulation. **Client does not own or directly control game state.**

**Reality (`client_main.cpp:47-61`):**
```cpp
float throttleInput = 0.0f;
// ... input handling ...
carSetInput(playerCar, throttleInput, turnInput, handbrakeInput);  // Direct manipulation
stepCommon();  // Steps physics directly
```

**Impact:** This is client-server in name only. True server-authoritative design would require:
- Input events sent to server
- Server simulation steps
- State snapshots sent to client
- Client-side prediction with rollback

### 2.4 No Input Abstraction

**Stated goal (AGENTS.md):**
> input generates events that manipulate state

**Reality:**
- No `InputEvent` struct or type
- No input queue system
- Input directly sets `car.throttleInput`, `car.turnInput`

**Impact:** Cannot support:
- Input recording for replay
- Input prediction/buffering
- Network transmission of inputs

### 2.5 AI Logic Entangled with Physics

**Location:** `common_main.cpp:14, 152-196`

The `updateAICars()` function is defined in `common_main.cpp` and mixes AI behavior with physics stepping. While game logic in common is correct per AGENTS.md, the AI:
- Has hardcoded magic numbers (`0.7f`, `0.6f`, `3.0f`, etc.)
- Is not easily configurable
- Cannot be replaced per-instance

### 2.6 No Entity Abstraction

**Stated goal (AGENTS.md):**
> New entity types should not require changes to core architecture

**Reality:**
- Only `Car` entity exists
- No base `Entity` class or interface
- Adding new entity types (e.g., projectiles, pickups) would require core changes

---

## 3. Maintainability Issues

### 3.1 Global Mutable State

**Location:** `common_main.cpp:7-12`
```cpp
static b2WorldId worldId;
static Car playerCar;
static std::vector<Car> aiCars;
static b2Vec2 arenaMin = {-20.0f, -15.0f};
static b2Vec2 arenaMax = {20.0f, 15.0f};
static std::vector<Obstacle> obstacles;
```

Issues:
- No encapsulation - any code can modify these
- No thread safety (relevant for future networking)
- Difficult to test in isolation
- Cannot run multiple simulations concurrently

### 3.2 Duplicate Collision Bit Definitions

| File | Definition |
|------|------------|
| `physics_settings.h:10-18` | `CollisionBits::StaticBit = 0x0001` |
| `car.h:25-29` | `CarCollisionBits::StaticBit = 0x0001` |

These serve different purposes but have the same value - potential collision bugs.

### 3.3 No Error Handling

- No `std::expected` usage (AGENTS.md suggests it for recoverable errors)
- No error return codes
- Box2D calls assume success

### 3.4 Dead Code

- `debug_draw.cpp` / `debug_draw.h` exists but is never used in `client_main.cpp`
- `src/CMakeLists.txt` defines file globs but doesn't appear to be used by root CMakeLists.txt

---

## 4. Build/Configuration Issues

### 4.1 Unused CMake Subdirectory

- `src/CMakeLists.txt` defines `SRC_SOURCE_FILES` and `SRC_HEADER_FILES` with `PARENT_SCOPE`
- Root `CMakeLists.txt` uses `file(GLOB_RECURSE)` directly - doesn't use the subdirectory
- The `src/CMakeLists.txt` is essentially dead code

### 4.2 Server Has No Exit Condition

`server_main.cpp:6-8`:
```cpp
while (true) {
    stepCommon();
}
```

No graceful shutdown mechanism - cannot terminate server process cleanly.

---

## 5. Extensibility Barriers

| Feature | Current State | Barrier |
|---------|---------------|---------|
| Networking | None | Requires complete rewrite |
| Serialization | None | Cannot store/load state |
| Input recording | None | No input abstraction |
| New entity types | None | No entity system |
| Multiple worlds | None | Global state |
| Configurable AI | None | Hardcoded in common |
| Runtime configuration | None | Magic numbers inline |

---

## 6. Checklist Summary

### Must Fix (Architecture-Breaking)
- [ ] Implement networking layer or remove claims from AGENTS.md
- [ ] Add serialization system for state snapshots
- [ ] Fix client to not directly control game state
- [ ] Create input abstraction layer

### Should Fix (Maintainability)
- [ ] Fix all header include order violations
- [ ] Replace `<assert.h>` with `<cassert>`
- [ ] Use `enum class` for `CollisionBits`
- [ ] Fix include guard naming (`physics_settings.h`)
- [ ] Remove magic numbers - use named constants
- [ ] Consolidate duplicate collision bit definitions
- [ ] Remove dead code (`debug_draw.cpp`, `src/CMakeLists.txt`)

### Nice to Have (Extensibility)
- [ ] Encapsulate global state in a `GameWorld` class
- [ ] Create entity base class/system
- [ ] Add error handling with `std::expected`
- [ ] Add graceful shutdown to server

---

## Conclusion

The project has a good CMake foundation and basic physics working, but the stated architectural goals are not reflected in the code. The client-server architecture exists in name only - there's no networking, no serialization, and the client directly controls game state. 

To meet the stated goal of being "extensible and reusable for future projects," significant refactoring is needed before adding features like multiplayer or rollback. The current architecture would require partial rewrites for each major feature addition.

**Recommendation:** Prioritize creating the networking/serialization abstraction layer now, before more game logic is added. Every line of code written against the current architecture increases technical debt.

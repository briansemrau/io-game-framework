# Tasks

## Phase 1: Top-Down Car Game Foundation
- [x] (none yet - starting fresh)

## Implementation Tasks
- [x] Create car entity with top-down physics in common module
- [x] Add car control input handling in client  
- [x] Add car rendering in client
- [x] Add arena boundaries (walls)
- [x] Build and verify the project compiles
- [x] Test car drives on screen

## Review - Phase 1
### What's Working
- Car physics with top-down driving (throttle, steering, drift)
- WASD/Arrow controls with handbrake (Space)
- Arena boundaries keep car on screen
- Both client and server build successfully

### Concerns
- Car physics may need tuning (turn speed, drift factor)
- No way to restart or reset the car if stuck
- No multiplayer/networking implementation yet (architecture supports it)
- Simple rectangle rendering, no sprites

## Next Phase
- [x] Tune car physics parameters for better feel
- [x] Add basic obstacle inside arena
- [x] Add multiple cars for demo/testing
- [ ] Consider adding simple car sprite/graphics

## Review - Phase 2
### What's Working
- Added 6 obstacles to the arena for collision testing
- Added 3 AI cars that drive around autonomously
- Player car (red) vs AI cars (blue)
- Full physics simulation working

### Concerns
- AI cars drive in circles/patterns, not intelligent
- No collision between player and AI cars yet
- Simple rectangle rendering, no sprites

## Phase 3: Game Improvements
- [x] Add reset key (R) to restart game
- [x] Improve AI behavior
- [x] Add car-to-car collision (already works via box2d)

## Phase 4: Damage System
- [ ] Add collision damage based on impact velocity (deferred - box2d v3 API issues)
- [ ] Show car health/color changes on damage
- [ ] Destroy cars when health reaches zero
- [ ] Add victory condition

Note: Collision damage requires box2d v3 contact events API. Deferred for now.

## Design Notes
- Top-down car uses raycast or custom movement (not standard platformer gravity)
- Car has: position, rotation, velocity, angular velocity, acceleration
- Controls: accelerate forward/backward, turn left/right, handbrake

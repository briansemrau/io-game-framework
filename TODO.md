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
- [ ] Tune car physics parameters for better feel
- [ ] Add basic obstacle inside arena
- [ ] Add multiple cars for demo/testing
- [ ] Consider adding simple car sprite/graphics

## Design Notes
- Top-down car uses raycast or custom movement (not standard platformer gravity)
- Car has: position, rotation, velocity, angular velocity, acceleration
- Controls: accelerate forward/backward, turn left/right, handbrake

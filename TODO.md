# TODO - Architectural Issues and Gaps

## High Priority

1. **No client-server networking** - Client and server are separate executables but have no network communication mechanism. They cannot synchronize game state.

2. **No entity/game state system** - The physics world exists but there are no game entities (players, vehicles, terrain). Commented-out code references `character`, `tileMap` that don't exist.

3. **Server infinite loop without timing** - `server_main.cpp:6` has `while(true)` with no sleep or frame rate limiting - will spin at 100% CPU with no way to exit.

4. **No cleanup/shutdown** - `common_main.cpp` creates a b2World but has no function to destroy it on shutdown.

5. **Input never applied** - `client_main.cpp:35` reads input but `character.CommandInput` is commented out; input has no effect on game state.

## Medium Priority

6. **Debug draw not used** - `debug_draw.cpp` implements `b2RaylibDebugDraw()` but it's never called in client_main (see commented line 56).

7. **Hardcoded physics timestep** - `common_main.cpp:19` hardcodes `deltaTime = 1.0f / 60.0f` with no configuration.

8. **No camera system** - No viewport/camera handling for rendering a larger world.

9. **No asset/resource system** - No texture, sprite, or sound loading infrastructure.

10. **Generic include guard** - `physics_settings.h:2` uses `PHYSICS_H` which is too common and could conflict.

11. **Unused include** - `common_main.cpp:2` includes `<cstddef>` but it's not used.

12. **No proper game loop interface** - `common_main` provides no way to query game state or entity positions.

## Low Priority

13. **No logging system** - No debug/logging infrastructure for troubleshooting.

14. **No configuration file** - All settings are hardcoded in source files.

15. **No error handling** - Physics callbacks silently return true with no diagnostics.

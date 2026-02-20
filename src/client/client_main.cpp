#include "raylib.h"
#include "box2d/box2d.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <format>
#include <iostream>
#include <numbers>
#include <type_traits>
#include <vector>
#include <chrono>

#include "physics_settings.h"

#include "game_state.h"
#include "debug_draw.h"
#include "renderer.h"

struct TrailPoint
{
    Vector2 position;
    float alpha;
};

using Seconds = std::chrono::duration<float, std::ratio<1>>;

int main() {
    constexpr int screenWidth = 1280;
    constexpr int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "Destruction Derby");
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor()));

    GameState game_state;
    game_state.init();

    Renderer renderer;
    RenderState render_state;

    Car& playerCar = game_state.getPlayerCar();
    std::vector<TrailPoint> playerTrail;
    b2DebugDraw debugDraw = b2RaylibDebugDraw();

    SetWindowTitle("Destruction Derby");

    float timescale = 1.0; // TODO get from server. maybe encapsulate in game state? or do we want some generic header data in the network data?

    // TODO do we want a network thread for receiving? I suppose we want to multithread decompression and deserialization (and decryption?)
    // then we need some thread to handle rapid resimulation. So we want another thread to handle resimulation to not stall the network thread.
    // We also want a primary update thread. The main thread is the rendering thread, which needs to be fed a game state copy at the beginning of each frame (if the state is dirty)
    // The primary update thread needs to be separate from the resim thread to not stall if the client gets too far behind. This will show the same as having a larger connection latency.

    auto previousTime = std::chrono::steady_clock::now();
    auto remainingTime = Seconds::zero();
    while (!WindowShouldClose()) {
        // The hands of time
        const auto currentTime = std::chrono::steady_clock::now();
        const Seconds elapsed = currentTime - previousTime;
        previousTime = currentTime;
        remainingTime += elapsed;

        // Window management I guess
        SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor()));

        // Input
        float mouseWheel = GetMouseWheelMove();
        if (mouseWheel != 0) {
            renderer.handleMouseWheel(mouseWheel);
        }

        float throttleInput = 0.0f;
        float turnInput = 0.0f;
        bool handbrakeInput = false;

        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) throttleInput += 1.0f;
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) throttleInput -= 1.0f;
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) turnInput -= 1.0f;
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) turnInput += 1.0f;
        if (IsKeyDown(KEY_SPACE)) handbrakeInput = true;
        
        if (IsKeyPressed(KEY_R)) game_state.reset();
        if (IsKeyPressed(KEY_G)) render_state.debugDrawEnabled = !render_state.debugDrawEnabled;
        
        render_state.handbrake = handbrakeInput;
        
        playerCar.setInput(throttleInput, turnInput, handbrakeInput);

        // TODO this is where we queue input events. they will be handled in the next timestep, and also synchronized with the server.
        // The rollback data will store the most recent server truth (including all other user's input events), and retain all input events across timesteps since that truth state so that they can be replayed in resim.
        
        // Input events can be stored as global events, or as state on entities. It depends on the side effects of the event.
        // Storing as entity state allows us to reduce bandwidth by implicitly passing it through spatial filtering.
        
        // Do work
        const auto current_timestep = Seconds(FixedTimestepDuration) * timescale;
        if (remainingTime >= current_timestep) {
            game_state.step();
            remainingTime -= current_timestep;
        }
        
        // Rendering
        b2Vec2 carPos = playerCar.getPosition();
        b2Vec2 velocity = playerCar.getVelocity();
        float speed = b2Length(velocity);
        
        if (speed > 5.0f || handbrakeInput) {
            Vector2 screenCenter = { (float)GetScreenWidth() / 2.0f, (float)GetScreenHeight() / 2.0f };
            Vector2 screenPos = {
                screenCenter.x + carPos.x * renderer.getZoom(),
                screenCenter.y + carPos.y * renderer.getZoom()
            };
            playerTrail.push_back({screenPos, 1.0f});
            if (playerTrail.size() > 60) playerTrail.erase(playerTrail.begin());
        }
        
        for (auto& point : playerTrail) {
            point.alpha -= 0.015f;
        }
        playerTrail.erase(
            std::remove_if(playerTrail.begin(), playerTrail.end(), [](const TrailPoint& p) { return p.alpha <= 0; }),
            playerTrail.end()
        );
        
        renderer.render(game_state, render_state);
    }

    CloseWindow();

    return 0;
}

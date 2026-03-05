#include "client_instance.h"

#include <cassert>
#include <chrono>

namespace raylib {
#include "raylib.h"
}

using Seconds = std::chrono::duration<float, std::ratio<1>>;

ClientInstance::ClientInstance() : m_networkClient(m_game) {}

void ClientInstance::run() {
    constexpr int screenWidth = 1280;
    constexpr int screenHeight = 720;

    raylib::InitWindow(screenWidth, screenHeight, "Test Game");
    raylib::SetWindowState(raylib::FLAG_WINDOW_RESIZABLE);
    auto lastRefreshRate = raylib::GetMonitorRefreshRate(raylib::GetCurrentMonitor());
    raylib::SetTargetFPS(lastRefreshRate);

    raylib::SetWindowTitle("Test Game");

    constexpr PeerID SERVER_ID = 12345;  // TODO: get from matchmaking server
    m_networkClient.connect("localhost", 9812, SERVER_ID);

    float timescale = 1.0;  // TODO get from server. maybe encapsulate in game state? or do we want some generic header
                            // data in the network data?

    // TODO do we want a network thread for receiving? I suppose we want to multithread decompression and
    // deserialization (and decryption?) then we need some thread to handle rapid resimulation. So we want another
    // thread to handle resimulation to not stall the network thread. We also want a primary update thread. The main
    // thread is the rendering thread, which needs to be fed a game state copy at the beginning of each frame (if the
    // state is dirty)
    // The primary update thread needs to be separate from the resim thread to not stall if the client
    // gets too far behind. This will show the same as having a larger connection latency.

    auto previousTime = std::chrono::steady_clock::now();
    auto remainingTime = Seconds::zero();
    while (!raylib::WindowShouldClose()) {
        // The hands of time
        const auto currentTime = std::chrono::steady_clock::now();
        const Seconds elapsed = currentTime - previousTime;
        previousTime = currentTime;
        remainingTime += elapsed;

        // Window management I guess
        auto nextRefreshRate = raylib::GetMonitorRefreshRate(raylib::GetCurrentMonitor());
        if (nextRefreshRate != lastRefreshRate) {
            raylib::SetTargetFPS(raylib::GetMonitorRefreshRate(raylib::GetCurrentMonitor()));
            lastRefreshRate = nextRefreshRate;
        }

        // Input
        handleInput();

        // Do work
        const auto current_timestep = Seconds(m_game.getFixedTimestepDuration()) * timescale;
        if (remainingTime >= current_timestep) {
            step();
            remainingTime -= current_timestep;
        }

        // Rendering
        render();
    }

    raylib::CloseWindow();
}

void ClientInstance::handleInput() {
    // TODO this is where we queue input events. they will be handled in the next timestep, and also synchronized with
    // the server. The rollback data will store the most recent server truth (including all other user's input events),
    // and retain all input events across timesteps since that truth state so that they can be replayed in resim.

    // Input events can be stored as global events, or as state on entities. It depends on the side effects of the
    // event. Storing as entity state allows us to reduce bandwidth by implicitly passing it through spatial filtering.

    float throttleInput = 0.0f;
    float turnInput = 0.0f;
    bool handbrakeInput = false;

    if (raylib::IsKeyDown(raylib::KEY_W) || raylib::IsKeyDown(raylib::KEY_UP)) throttleInput += 1.0f;
    if (raylib::IsKeyDown(raylib::KEY_S) || raylib::IsKeyDown(raylib::KEY_DOWN)) throttleInput -= 1.0f;
    if (raylib::IsKeyDown(raylib::KEY_A) || raylib::IsKeyDown(raylib::KEY_LEFT)) turnInput -= 1.0f;
    if (raylib::IsKeyDown(raylib::KEY_D) || raylib::IsKeyDown(raylib::KEY_RIGHT)) turnInput += 1.0f;
    if (raylib::IsKeyDown(raylib::KEY_SPACE)) handbrakeInput = true;

    if (raylib::IsKeyPressed(raylib::KEY_G)) m_renderState.debugDrawEnabled = !m_renderState.debugDrawEnabled;

    // m_gameState.getPlayerCar().setInput(throttleInput, turnInput, handbrakeInput);
}

void ClientInstance::step() { m_game.step(); }

void ClientInstance::render() { m_renderer.render(m_game.getState(), m_renderState); }

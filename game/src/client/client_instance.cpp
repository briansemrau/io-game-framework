#include "client_instance.h"

#include <cassert>
#include <chrono>

namespace raylib {
#include "raylib.h"
}

using Seconds = std::chrono::duration<float, std::ratio<1>>;

ClientInstance::ClientInstance(bool p_isLocal)
    : m_game(p_isLocal)
    , m_networkClient(m_game)
    , m_renderer(m_game) {}

void ClientInstance::run() {
    constexpr int screenWidth = 1280;
    constexpr int screenHeight = 720;

    m_renderer.init(screenWidth, screenHeight);

    raylib::InitWindow(screenWidth, screenHeight, "Test Game");
    raylib::SetWindowState(raylib::FLAG_WINDOW_RESIZABLE);
    auto lastRefreshRate = raylib::GetMonitorRefreshRate(raylib::GetCurrentMonitor());
    raylib::SetTargetFPS(lastRefreshRate);

    raylib::SetWindowTitle("Test Game");

    if (!m_game.isServer()) {
        constexpr PeerID SERVER_ID = 12345;  // TODO: get from matchmaking server
        m_networkClient.connect("localhost", 9812, SERVER_ID);
    }

    if (m_game.isServer()) {
        m_game.spawnTank({}, {}, m_localPeerId);
    }

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
    float throttleInput = 0.0f;
    float turnInput = 0.0f;
    bool shootInput = false;

    if (raylib::IsKeyDown(raylib::KEY_W) || raylib::IsKeyDown(raylib::KEY_UP)) throttleInput += 1.0f;
    if (raylib::IsKeyDown(raylib::KEY_S) || raylib::IsKeyDown(raylib::KEY_DOWN)) throttleInput -= 1.0f;
    if (raylib::IsKeyDown(raylib::KEY_A) || raylib::IsKeyDown(raylib::KEY_LEFT)) turnInput -= 1.0f;
    if (raylib::IsKeyDown(raylib::KEY_D) || raylib::IsKeyDown(raylib::KEY_RIGHT)) turnInput += 1.0f;
    if (raylib::IsKeyPressed(raylib::KEY_SPACE)) shootInput = true;

    if (raylib::IsKeyPressed(raylib::KEY_G)) m_renderer.getRenderState().debugDrawEnabled = !m_renderer.getRenderState().debugDrawEnabled;

    // Spawn test entities
    if (raylib::IsKeyPressed(raylib::KEY_C)) {
        m_game.spawnCollectible(b2Vec2(10.0f, 10.0f));
    }
    if (raylib::IsKeyPressed(raylib::KEY_X)) {
        m_game.spawnDestructible(b2Vec2(20.0f, 0.0f));
    }

    InputState input;
    input.throttle = throttleInput;
    input.turn = turnInput;
    input.shoot = shootInput;

    m_game.queueInput(input, m_localPeerId);
}

void ClientInstance::step() { m_game.step(); }

void ClientInstance::render() { m_renderer.render(); }

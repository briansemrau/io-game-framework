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

#include "common_main.h"
#include "debug_draw.h"

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

    initCommon();
    Car& playerCar = getPlayerCar();
    float zoom = 30.0f;
    std::vector<TrailPoint> playerTrail;
    bool debugDrawEnabled = false;
    b2DebugDraw debugDraw = b2RaylibDebugDraw();

    SetWindowTitle("Destruction Derby - Drive with WASD, Handbrake: Space, Reset: R");

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
            zoom += mouseWheel * 5.0f;
            if (zoom < 10.0f) zoom = 10.0f;
            if (zoom > 60.0f) zoom = 60.0f;
        }

        float throttleInput = 0.0f;
        float turnInput = 0.0f;
        bool handbrakeInput = false;

        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) throttleInput += 1.0f;
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) throttleInput -= 1.0f;
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) turnInput -= 1.0f;
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) turnInput += 1.0f;
        if (IsKeyDown(KEY_SPACE)) handbrakeInput = true;
        
        if (IsKeyPressed(KEY_R)) resetGame();
        if (IsKeyPressed(KEY_G)) debugDrawEnabled = !debugDrawEnabled;
        
        carSetInput(playerCar, throttleInput, turnInput, handbrakeInput);

        // TODO this is where we queue input events. they will be handled in the next timestep, and also synchronized with the server.
        // The rollback data will store the most recent server truth (including all other user's input events), and retain all input events across timesteps since that truth state so that they can be replayed in resim.
        
        // Input events can be stored as global events, or as state on entities. It depends on the side effects of the event.
        // Storing as entity state allows us to reduce bandwidth by implicitly passing it through spatial filtering.
        
        // Do work
        const auto current_timestep = Seconds(fixed_timestep_duration) * timescale;
        if (remainingTime >= current_timestep) {
            fixedTimestep();
            remainingTime -= current_timestep;
        }
        
        // Rendering
        b2Vec2 carPos = b2Body_GetTransform(playerCar.bodyId).p;
        b2Vec2 velocity = b2Body_GetLinearVelocity(playerCar.bodyId);
        float speed = b2Length(velocity);
        
        if (speed > 5.0f || handbrakeInput)
        {
            Vector2 screenCenter = { (float)GetScreenWidth() / 2.0f, (float)GetScreenHeight() / 2.0f };
            Vector2 screenPos = {
                screenCenter.x + carPos.x * zoom,
                screenCenter.y + carPos.y * zoom
            };
            playerTrail.push_back({screenPos, 1.0f});
            if (playerTrail.size() > 60) playerTrail.erase(playerTrail.begin());
        }
        
        for (auto& point : playerTrail)
        {
            point.alpha -= 0.015f;
        }
        playerTrail.erase(
            std::remove_if(playerTrail.begin(), playerTrail.end(), [](const TrailPoint& p) { return p.alpha <= 0; }),
            playerTrail.end()
        );
        
        BeginDrawing();
        {
            ClearBackground({40, 45, 50, 255});
            
            b2Rot carRot = b2Body_GetTransform(playerCar.bodyId).q;
            float carAngle = b2Rot_GetAngle(carRot);
            
            Vector2 screenCenter = { (float)GetScreenWidth() / 2.0f, (float)GetScreenHeight() / 2.0f };
            Vector2 carScreenPos = {
                screenCenter.x + carPos.x * zoom,
                screenCenter.y + carPos.y * zoom
            };

            float arenaW = 40.0f * zoom;
            float arenaH = 30.0f * zoom;
            
            Rectangle arenaRect = {
                screenCenter.x - arenaW / 2.0f,
                screenCenter.y - arenaH / 2.0f,
                arenaW,
                arenaH
            };
            DrawRectangleLinesEx(arenaRect, 4.0f, {80, 90, 100, 255});

            float gridSize = 5.0f;
            for (float x = -20.0f; x <= 20.0f; x += gridSize)
            {
                Vector2 start = {screenCenter.x + x * zoom, screenCenter.y - 15.0f * zoom};
                Vector2 end = {screenCenter.x + x * zoom, screenCenter.y + 15.0f * zoom};
                DrawLineV(start, end, {50, 55, 60, 100});
            }
            for (float y = -15.0f; y <= 15.0f; y += gridSize)
            {
                Vector2 start = {screenCenter.x - 20.0f * zoom, screenCenter.y + y * zoom};
                Vector2 end = {screenCenter.x + 20.0f * zoom, screenCenter.y + y * zoom};
                DrawLineV(start, end, {50, 55, 60, 100});
            }

            for (const auto& point : playerTrail)
            {
                DrawCircleV(point.position, 3.0f, {200, 60, 60, (unsigned char)(point.alpha * 255)});
            }

            for (const auto& obstacle : getObstacles())
            {
                Vector2 obstacleScreenPos = {
                    screenCenter.x + obstacle.position.x * zoom,
                    screenCenter.y + obstacle.position.y * zoom
                };
                float obstacleScreenW = obstacle.halfSize.x * 2.0f * zoom;
                float obstacleScreenH = obstacle.halfSize.y * 2.0f * zoom;
                DrawRectanglePro(
                    {obstacleScreenPos.x, obstacleScreenPos.y, obstacleScreenW, obstacleScreenH},
                    {obstacleScreenW / 2.0f, obstacleScreenH / 2.0f},
                    obstacle.angle * (180.0f / std::numbers::pi),
                    {100, 100, 110, 255}
                );
            }

            float carScreenW = playerCar.width * zoom;
            float carScreenH = playerCar.height * zoom;
            
            Color playerCarColor = handbrakeInput ? Color{255, 150, 50, 255} : Color{200, 60, 60, 255};
            
            DrawRectanglePro(
                {carScreenPos.x, carScreenPos.y, carScreenW, carScreenH},
                {carScreenW / 2.0f, carScreenH / 2.0f},
                carAngle * (180.0f / std::numbers::pi),
                playerCarColor
            );

            Vector2 frontDir = {
                sinf(carAngle),
                cosf(carAngle)
            };
            Vector2 frontLightPos = {
                carScreenPos.x + frontDir.x * carScreenH / 2.0f,
                carScreenPos.y + frontDir.y * carScreenH / 2.0f
            };
            DrawCircleV(frontLightPos, 4.0f, {255, 255, 150, 255});

            for (size_t i = 0; i < getAICars().size(); i++)
            {
                const auto& aiCar = getAICars()[i];
                b2Vec2 aiCarPos = b2Body_GetTransform(aiCar.bodyId).p;
                b2Rot aiCarRot = b2Body_GetTransform(aiCar.bodyId).q;
                float aiCarAngle = b2Rot_GetAngle(aiCarRot);

                Vector2 aiCarScreenPos = {
                    screenCenter.x + aiCarPos.x * zoom,
                    screenCenter.y + aiCarPos.y * zoom
                };

                float aiCarScreenW = aiCar.width * zoom;
                float aiCarScreenH = aiCar.height * zoom;

                Color aiColors[] = {
                    {60, 100, 200, 255},
                    {100, 60, 200, 255},
                    {60, 200, 100, 255},
                    {200, 100, 60, 255},
                    {200, 60, 180, 255}
                };
                Color aiColor = aiColors[i % 5];

                DrawRectanglePro(
                    {aiCarScreenPos.x, aiCarScreenPos.y, aiCarScreenW, aiCarScreenH},
                    {aiCarScreenW / 2.0f, aiCarScreenH / 2.0f},
                    aiCarAngle * (180.0f / std::numbers::pi),
                    aiColor
                );
            }

            const char* controls = "WASD/Arrows: Drive | Space: Handbrake | R: Reset | G: Debug | Scroll: Zoom";
            DrawText(controls, 10, GetScreenHeight() - 30, 20, LIGHTGRAY);
            
            DrawFPS(GetScreenWidth() - 100, 10);
            DrawText(std::format(" {:.2f}", GetFrameTime()).c_str(), GetScreenWidth() - 100, 30, 20, GRAY);
            DrawText(std::format("x: {:.1f} y: {:.1f}", carPos.x, carPos.y).c_str(), GetScreenWidth() - 200, 50, 20, GRAY);
            DrawText(std::format("Zoom: {:.0f}", zoom).c_str(), GetScreenWidth() - 200, 70, 20, GRAY);
            DrawText(std::format("Speed: {:.1f}", speed).c_str(), 10, 10, 20, LIGHTGRAY);
            
            int aiAlive = 0;
            for (const auto& aiCar : getAICars())
            {
                if (aiCar.health > 0) aiAlive++;
            }
            DrawText(std::format("Enemies: {}", aiAlive).c_str(), GetScreenWidth() - 200, 90, 20, GRAY);
            
            float miniMapSize = 100.0f;
            float miniMapScale = 2.0f;
            Vector2 miniMapPos = {GetScreenWidth() - miniMapSize - 20, GetScreenHeight() - miniMapSize - 20};
            DrawRectangle((int)miniMapPos.x, (int)miniMapPos.y, (int)miniMapSize, (int)miniMapSize, {20, 25, 30, 150});
            DrawRectangleLines((int)miniMapPos.x, (int)miniMapPos.y, (int)miniMapSize, (int)miniMapSize, {80, 90, 100, 255});
            
            b2Vec2 playerPos = b2Body_GetTransform(playerCar.bodyId).p;
            DrawCircleV({miniMapPos.x + miniMapSize/2 + playerPos.x * miniMapScale, miniMapPos.y + miniMapSize/2 + playerPos.y * miniMapScale}, 4.0f, {255, 50, 50, 255});
            
            for (const auto& aiCar : getAICars())
            {
                b2Vec2 aiPos = b2Body_GetTransform(aiCar.bodyId).p;
                Vector2 aiMiniPos = {
                    miniMapPos.x + miniMapSize/2 + aiPos.x * miniMapScale,
                    miniMapPos.y + miniMapSize/2 + aiPos.y * miniMapScale
                };
                DrawCircleV(aiMiniPos, 3.0f, {50, 50, 255, 255});
            }

            if (debugDrawEnabled)
            {
                // b2World_DrawWorld(getWorldId(), &debugDraw); // TODO what is the right function?
            }
        }
        EndDrawing();
    }

    CloseWindow();

    return 0;
}

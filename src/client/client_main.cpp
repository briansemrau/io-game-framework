#include "raylib.h"
#include <vector>

#include "box2d/box2d.h"
#include <iostream>
#include <format>
#include <assert.h>
#include <algorithm>
#include <type_traits>

#include "physics_settings.h"

#include "common_main.h"

struct TrailPoint
{
    Vector2 position;
    float alpha;
};

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

    while (!WindowShouldClose()) {
        SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor()));

        float mouseWheel = GetMouseWheelMove();
        if (mouseWheel != 0)
        {
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
        
        carSetInput(playerCar, throttleInput, turnInput, handbrakeInput);
        
        stepCommon();
        
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
            if (playerTrail.size() > 30) playerTrail.erase(playerTrail.begin());
        }
        
        for (auto& point : playerTrail)
        {
            point.alpha -= 0.03f;
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
                    obstacle.angle * (180.0f / 3.14159f),
                    {100, 100, 110, 255}
                );
            }

            float carScreenW = playerCar.width * zoom;
            float carScreenH = playerCar.height * zoom;
            
            DrawRectanglePro(
                {carScreenPos.x, carScreenPos.y, carScreenW, carScreenH},
                {carScreenW / 2.0f, carScreenH / 2.0f},
                carAngle * (180.0f / 3.14159f),
                {200, 60, 60, 255}
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

            for (const auto& aiCar : getAICars())
            {
                b2Vec2 aiCarPos = b2Body_GetTransform(aiCar.bodyId).p;
                b2Rot aiCarRot = b2Body_GetTransform(aiCar.bodyId).q;
                float aiCarAngle = b2Rot_GetAngle(aiCarRot);

                Vector2 aiCarScreenPos = {
                    screenCenter.x + aiCarPos.x * zoom,
                    screenCenter.y + aiCarPos.y * zoom
                };

                float aiCarScreenW = aiCar.width * zoom;
                float aiCarScreenH = aiCar.height * zoom;

                DrawRectanglePro(
                    {aiCarScreenPos.x, aiCarScreenPos.y, aiCarScreenW, aiCarScreenH},
                    {aiCarScreenW / 2.0f, aiCarScreenH / 2.0f},
                    aiCarAngle * (180.0f / 3.14159f),
                    {60, 100, 200, 255}
                );
            }

            const char* controls = "WASD/Arrows: Drive | Space: Handbrake | R: Reset | Scroll: Zoom";
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
        }
        EndDrawing();
    }

    CloseWindow();

    return 0;
}

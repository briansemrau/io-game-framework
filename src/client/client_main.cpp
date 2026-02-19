#include "raylib.h"
#include "box2d/box2d.h"
#include <iostream>
#include <format>
#include <assert.h>
#include <algorithm>
#include <type_traits>

#include "physics_settings.h"

#include "common_main.h"

int main() {
    constexpr int screenWidth = 800;
    constexpr int screenHeight = 600;

    InitWindow(screenWidth, screenHeight, "Destruction Derby");
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor()));

    initCommon();

    while (!WindowShouldClose()) {
        SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor()));

        // Character movement input
        float throttleInput = 0.0f;
        float climbingInput = 0.0f;
        bool jumpInput = false;
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) throttleInput -= 1.0f;
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) throttleInput += 1.0f;
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP) || IsKeyDown(KEY_SPACE)) climbingInput += 1.0f;
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) throttleInput -= 1.0f;
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP) || IsKeyDown(KEY_SPACE)) jumpInput = true;
        // character.CommandInput(throttleInput, jumpInput, climbingInput);
        
        stepCommon();
        
        BeginDrawing();
        {
            ClearBackground(RAYWHITE);
            
            // Render tile map
            // tileMap.Render();
            
            // Render character
            // character.Render();
            // debugDraw.DrawSolidCapsuleFcn(
            //     b2TransformPoint(character.transform, character.capsule.center1),
            //     b2TransformPoint(character.transform, character.capsule.center2),
            //     character.capsule.radius,
            //     b2_colorAqua,
            //     nullptr
            // );

            // b2World_Draw(worldId, &debugDraw);

            // DrawText(std::format("x={:.2f}, y={:.2f}, G={:d}", character.transform.p.x, character.transform.p.y, character.onGround).c_str(), 10, 10, 20, BLACK);
            
            // static const std::vector<std::string> toolStrings = {
            //     "Dig",
            //     "Place",
            //     "Ladder"
            // };
            // const std::string toolStr = toolStrings[(int)currentTool];
            // DrawText(std::format("Tool: {} (1=Dig, 2=Place, 3=Ladder)", toolStr).c_str(), 10, 35, 20, DARKGRAY);

            DrawFPS(GetScreenWidth() - 100, 10);
            DrawText(std::format("{:.4f}", GetFrameTime()).c_str(), GetScreenWidth() - 100, 30, 20, GRAY);
        }
        EndDrawing();
    }

    CloseWindow();

    return 0;
}
#include "renderer.h"

#include <cmath>
#include <format>
#include <numbers>

#include "game.h"
#include "raylib.h"

void Renderer::render(const GameState& gameState, const RenderState& renderState) {
    float zoom = 1;

    BeginDrawing();
    {
        ClearBackground({40, 45, 50, 255});

        Vector2 screenCenter = {(float)GetScreenWidth() / 2.0f, (float)GetScreenHeight() / 2.0f};

        float arenaW = GameState::ArenaWidth * zoom;
        float arenaH = GameState::ArenaHeight * zoom;

        Rectangle arenaRect = {screenCenter.x - arenaW / 2.0f, screenCenter.y - arenaH / 2.0f, arenaW, arenaH};
        DrawRectangleLinesEx(arenaRect, 4.0f, {80, 90, 100, 255});

        float gridSize = 5.0f;
        for (float x = GameState::ArenaMinX; x <= GameState::ArenaMaxX; x += gridSize) {
            Vector2 start = {screenCenter.x + x * zoom, screenCenter.y + GameState::ArenaMinY * zoom};
            Vector2 end = {screenCenter.x + x * zoom, screenCenter.y + GameState::ArenaMaxY * zoom};
            DrawLineV(start, end, {50, 55, 60, 100});
        }
        for (float y = GameState::ArenaMinY; y <= GameState::ArenaMaxY; y += gridSize) {
            Vector2 start = {screenCenter.x + GameState::ArenaMinX * zoom, screenCenter.y + y * zoom};
            Vector2 end = {screenCenter.x + GameState::ArenaMaxX * zoom, screenCenter.y + y * zoom};
            DrawLineV(start, end, {50, 55, 60, 100});
        }

        const char* controls = "WASD/Arrows: Drive | Space: Handbrake | R: Reset | G: Debug | Scroll: Zoom";
        DrawText(controls, 10, GetScreenHeight() - 30, 20, LIGHTGRAY);

        DrawFPS(GetScreenWidth() - 100, 10);
        DrawText(std::format(" {:.2f}", GetFrameTime()).c_str(), GetScreenWidth() - 100, 30, 20, GRAY);

        // DrawText(std::format("x: {:.1f} y: {:.1f}", carPos.x, carPos.y).c_str(), GetScreenWidth() - 200, 50, 20, GRAY);
        // DrawText(std::format("Zoom: {:.0f}", zoom).c_str(), GetScreenWidth() - 200, 70, 20, GRAY);
        // DrawText(std::format("Speed: {:.1f}", speed).c_str(), 10, 10, 20, LIGHTGRAY);

        // float miniMapSize = 100.0f;
        // float miniMapScale = 2.0f;
        // Vector2 miniMapPos = {GetScreenWidth() - miniMapSize - 20, GetScreenHeight() - miniMapSize - 20};
        // DrawRectangle((int)miniMapPos.x, (int)miniMapPos.y, (int)miniMapSize, (int)miniMapSize, {20, 25, 30, 150});
        // DrawRectangleLines((int)miniMapPos.x, (int)miniMapPos.y, (int)miniMapSize, (int)miniMapSize, {80, 90, 100, 255});

        // b2Vec2 playerPos = playerCar.getPosition();
        // DrawCircleV({miniMapPos.x + miniMapSize / 2 + playerPos.x * miniMapScale, miniMapPos.y + miniMapSize / 2 + playerPos.y * miniMapScale}, 4.0f, {255, 50, 50, 255});

        if (renderState.debugDrawEnabled) {
            // Debug draw requires passing b2DebugDraw from client_main.cpp
            // TODO: Implement proper debug draw by passing debugDraw to renderer
        }
    }
    EndDrawing();
}

// void Renderer::renderCar(const Car& car, Vector2 screenPos, float zoom, Color color) {
//     float carAngle = car.getAngle();

//     float carScreenW = car.width * zoom;
//     float carScreenH = car.height * zoom;

//     DrawRectanglePro({screenPos.x, screenPos.y, carScreenW, carScreenH}, {carScreenW / 2.0f, carScreenH / 2.0f}, carAngle * (180.0f / std::numbers::pi), color);
// }

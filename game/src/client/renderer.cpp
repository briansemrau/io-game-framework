#include "renderer.h"

#include <cmath>
#include <format>
#include <numbers>

#include "game.h"

namespace raylib {
#include "raylib.h"
}

void Renderer::render(const GameState& gameState) {
    float zoom = 1;

    raylib::BeginDrawing();
    {
        raylib::ClearBackground({40, 45, 50, 255});

        raylib::Vector2 screenCenter = {(float)raylib::GetScreenWidth() / 2.0f, (float)raylib::GetScreenHeight() / 2.0f};

        const char* controls = "WASD/Arrows: Drive | Space: Handbrake | R: Reset | G: Debug | Scroll: Zoom";
        raylib::DrawText(controls, 10, raylib::GetScreenHeight() - 30, 20, raylib::LIGHTGRAY);

        raylib::DrawFPS(raylib::GetScreenWidth() - 100, 10);
        raylib::DrawText(std::format(" {:.2f}", raylib::GetFrameTime()).c_str(), raylib::GetScreenWidth() - 100, 30, 20, raylib::GRAY);

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

        if (m_renderState.debugDrawEnabled) {
            // Debug draw requires passing b2DebugDraw from client_main.cpp
            // TODO: Implement proper debug draw by passing debugDraw to renderer
        }
    }
    raylib::EndDrawing();
}

// void Renderer::renderCar(const Car& car, Vector2 screenPos, float zoom, Color color) {
//     float carAngle = car.getAngle();

//     float carScreenW = car.width * zoom;
//     float carScreenH = car.height * zoom;

//     DrawRectanglePro({screenPos.x, screenPos.y, carScreenW, carScreenH}, {carScreenW / 2.0f, carScreenH / 2.0f}, carAngle * (180.0f / std::numbers::pi), color);
// }

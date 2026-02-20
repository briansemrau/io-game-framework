#include "renderer.h"

#include "raylib.h"

#include <cmath>
#include <format>
#include <numbers>

void Renderer::render(const GameState& gameState, const RenderState& renderState) {
    const Car& playerCar = gameState.getPlayerCar();
    b2Vec2 carPos = playerCar.getPosition();
    b2Vec2 velocity = playerCar.getVelocity();
    float speed = b2Length(velocity);
    float zoom = getZoom();

    BeginDrawing();
    {
        ClearBackground({40, 45, 50, 255});
        
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
        for (float x = -20.0f; x <= 20.0f; x += gridSize) {
            Vector2 start = {screenCenter.x + x * zoom, screenCenter.y - 15.0f * zoom};
            Vector2 end = {screenCenter.x + x * zoom, screenCenter.y + 15.0f * zoom};
            DrawLineV(start, end, {50, 55, 60, 100});
        }
        for (float y = -15.0f; y <= 15.0f; y += gridSize) {
            Vector2 start = {screenCenter.x - 20.0f * zoom, screenCenter.y + y * zoom};
            Vector2 end = {screenCenter.x + 20.0f * zoom, screenCenter.y + y * zoom};
            DrawLineV(start, end, {50, 55, 60, 100});
        }

        for (const auto& obstacle : gameState.getObstacles()) {
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

        renderPlayerCar(playerCar, carScreenPos, zoom, renderState.handbrake);

        for (size_t i = 0; i < gameState.getAICars().size(); i++) {
            const auto& aiCar = gameState.getAICars()[i];
            b2Vec2 aiCarPos = b2Body_GetTransform(aiCar.bodyId).p;

            Vector2 aiCarScreenPos = {
                screenCenter.x + aiCarPos.x * zoom,
                screenCenter.y + aiCarPos.y * zoom
            };

            Color aiColors[] = {
                {60, 100, 200, 255},
                {100, 60, 200, 255},
                {60, 200, 100, 255},
                {200, 100, 60, 255},
                {200, 60, 180, 255}
            };
            Color aiColor = aiColors[i % 5];

            renderCar(aiCar, aiCarScreenPos, zoom, aiColor);
        }

        const char* controls = "WASD/Arrows: Drive | Space: Handbrake | R: Reset | G: Debug | Scroll: Zoom";
        DrawText(controls, 10, GetScreenHeight() - 30, 20, LIGHTGRAY);
        
        DrawFPS(GetScreenWidth() - 100, 10);
        DrawText(std::format(" {:.2f}", GetFrameTime()).c_str(), GetScreenWidth() - 100, 30, 20, GRAY);
        DrawText(std::format("x: {:.1f} y: {:.1f}", carPos.x, carPos.y).c_str(), GetScreenWidth() - 200, 50, 20, GRAY);
        DrawText(std::format("Zoom: {:.0f}", zoom).c_str(), GetScreenWidth() - 200, 70, 20, GRAY);
        DrawText(std::format("Speed: {:.1f}", speed).c_str(), 10, 10, 20, LIGHTGRAY);
        
        int aiAlive = 0;
        for (const auto& aiCar : gameState.getAICars()) {
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
        
        for (const auto& aiCar : gameState.getAICars()) {
            b2Vec2 aiPos = b2Body_GetTransform(aiCar.bodyId).p;
            Vector2 aiMiniPos = {
                miniMapPos.x + miniMapSize/2 + aiPos.x * miniMapScale,
                miniMapPos.y + miniMapSize/2 + aiPos.y * miniMapScale
            };
            DrawCircleV(aiMiniPos, 3.0f, {50, 50, 255, 255});
        }

        if (renderState.debugDrawEnabled) {
            // b2World_DrawWorld(getWorldId(), &debugDraw); // TODO what is the right function?
        }
    }
    EndDrawing();
}

void Renderer::setZoom(float zoom) {
    m_zoom = zoom;
}

float Renderer::getZoom() const {
    return m_zoom;
}

void Renderer::handleMouseWheel(float delta) {
    m_zoom += delta * ZoomDelta;
    if (m_zoom < MinZoom) m_zoom = MinZoom;
    if (m_zoom > MaxZoom) m_zoom = MaxZoom;
}

void Renderer::renderCar(const Car& car, Vector2 screenPos, float zoom, Color color) {
    b2Rot carRot = b2Body_GetTransform(car.bodyId).q;
    float carAngle = b2Rot_GetAngle(carRot);

    float carScreenW = car.width * zoom;
    float carScreenH = car.height * zoom;

    DrawRectanglePro(
        {screenPos.x, screenPos.y, carScreenW, carScreenH},
        {carScreenW / 2.0f, carScreenH / 2.0f},
        carAngle * (180.0f / std::numbers::pi),
        color
    );
}

void Renderer::renderPlayerCar(const Car& car, Vector2 screenPos, float zoom, bool handbrake) {
    Color playerCarColor = handbrake ? Color{255, 150, 50, 255} : Color{200, 60, 60, 255};
    renderCar(car, screenPos, zoom, playerCarColor);

    b2Rot carRot = b2Body_GetTransform(car.bodyId).q;
    float carAngle = b2Rot_GetAngle(carRot);
    float carScreenH = car.height * zoom;

    Vector2 frontDir = {
        sinf(carAngle),
        cosf(carAngle)
    };
    Vector2 frontLightPos = {
        screenPos.x + frontDir.x * carScreenH / 2.0f,
        screenPos.y + frontDir.y * carScreenH / 2.0f
    };
    DrawCircleV(frontLightPos, 4.0f, {255, 255, 150, 255});
}

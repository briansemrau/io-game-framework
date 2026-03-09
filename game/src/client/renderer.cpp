// clang-format off
#include <cmath>
#include <numbers>
// clang-format on

#include "renderer.h"

namespace raylib {
#include "raylib.h"
}

GameRenderer::GameRenderer(const Game &game) : m_game(game) {}

void GameRenderer::init(int width, int height) {
    m_windowWidth = width;
    m_windowHeight = height;
    m_renderState.camera.target = { 0.0f, 0.0f };
    m_renderState.camera.offset = { static_cast<float>(width) / 2.0f, static_cast<float>(height) / 2.0f };
    m_renderState.camera.rotation = 0.0f;
    m_renderState.camera.zoom = 10.0f;
}

void GameRenderer::shutdown() {}

void GameRenderer::beginFrame() {
    raylib::BeginDrawing();
    raylib::ClearBackground({ 40, 45, 50, 255 });
    raylib::BeginMode2D(m_renderState.camera);
}

void GameRenderer::endFrame() {
    raylib::EndMode2D();
    renderUI();
    raylib::EndDrawing();
}

void GameRenderer::updateCamera(float targetX, float targetY) { m_renderState.camera.target = { targetX, targetY }; }

void GameRenderer::render() {
    beginFrame();

    bool foundPlayer = false;
    m_game.getState().registry.view<Transform, Tank>().each([this, &foundPlayer](const Transform &transform, const Tank &tank) {
        if (tank.isPlayer) {
            m_renderState.camera.target = { static_cast<float>(transform.position.x), static_cast<float>(transform.position.y) };
            foundPlayer = true;
        }
    });

    renderGameContent();

    endFrame();
}

void GameRenderer::renderGameContent() {
    m_game.getState().registry.view<entt::entity, Transform, Tank>().each(
        [this](entt::entity entity, const Transform &transform, const Tank &tank) { renderTank(entity, transform, tank); });

    m_game.getState().registry.view<Transform, Bullet>().each([this](const Transform &transform, const Bullet &bullet) { renderBullet(transform, bullet); });

    m_game.getState().registry.view<Transform, Collectible>().each(
        [this](const Transform &transform, const Collectible &collectible) { renderCollectible(transform, collectible); });

    m_game.getState().registry.view<Transform, Destructible>().each(
        [this](const Transform &transform, const Destructible &destructible) { renderDestructible(transform, destructible); });
}

void GameRenderer::renderUI() {
    const char *controls = "WASD: Move | Space: Shoot | C: Spawn Collectible | X: Spawn Destructible | G: Debug";
    raylib::DrawText(controls, 10, raylib::GetScreenHeight() - 30, 20, raylib::LIGHTGRAY);

    raylib::DrawFPS(raylib::GetScreenWidth() - 100, 10);
}

void GameRenderer::renderTank(entt::entity entity, const Transform &transform, const Tank &tank) {
    raylib::Vector2 pos{ static_cast<float>(transform.position.x), static_cast<float>(transform.position.y) };
    float angle = transform.rotation * (180.0f / std::numbers::pi_v<float>);

    raylib::Color color = tank.isPlayer ? raylib::GREEN : raylib::BLUE;
    raylib::DrawRectanglePro({ pos.x - 1.5f, pos.y - 1.5f, 3.0f, 3.0f }, { pos.x, pos.y }, angle, color);

    raylib::DrawCircleV(pos, 0.8f, raylib::DARKGREEN);

    raylib::Vector2 cannonTip = { pos.x + std::cosf(transform.rotation) * 2.5f, pos.y + std::sinf(transform.rotation) * 2.5f };
    raylib::DrawLineV(pos, cannonTip, raylib::DARKBROWN);
}

void GameRenderer::renderBullet(const Transform &transform, const Bullet &bullet) {
    raylib::Vector2 pos{ static_cast<float>(transform.position.x), static_cast<float>(transform.position.y) };
    raylib::DrawCircleV(pos, 0.3f, raylib::YELLOW);
}

void GameRenderer::renderCollectible(const Transform &transform, const Collectible &collectible) {
    if (!collectible.active) return;

    raylib::Vector2 pos{ static_cast<float>(transform.position.x), static_cast<float>(transform.position.y) };
    raylib::DrawCircleV(pos, collectible.radius, raylib::GOLD);
    raylib::DrawCircleLinesV(pos, collectible.radius, raylib::ORANGE);
}

void GameRenderer::renderDestructible(const Transform &transform, const Destructible &destructible) {
    if (!destructible.active) return;

    raylib::Vector2 pos{ static_cast<float>(transform.position.x), static_cast<float>(transform.position.y) };

    float healthRatio = destructible.health / destructible.maxHealth;
    raylib::Color color = raylib::Fade(raylib::Color{ (unsigned char)(200 * healthRatio + 50), (unsigned char)(50 + 150 * (1.0f - healthRatio)), 50, 255 }, 0.8f);

    raylib::DrawCircleV(pos, destructible.radius, color);
    raylib::DrawCircleLinesV(pos, destructible.radius, raylib::WHITE);

    float barWidth = destructible.radius * 2.0f;
    float barHeight = 0.4f;
    raylib::DrawRectangle(pos.x - destructible.radius, pos.y - destructible.radius - 0.6f, barWidth * healthRatio, barHeight, raylib::GREEN);
}
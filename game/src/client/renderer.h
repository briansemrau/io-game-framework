#ifndef RENDERER_H
#define RENDERER_H

namespace raylib {
#include "raylib.h"
}

#include "game.h"
#include "tank_components.h"

struct RenderState {
    bool debugDrawEnabled = false;
    raylib::Camera2D camera;
};

class GameRenderer {
public:
    GameRenderer(const Game &game);
    ~GameRenderer() = default;

    void init(int width, int height);
    void shutdown();
    void beginFrame();
    void endFrame();
    void updateCamera(float targetX, float targetY);

    void render();

    RenderState &getRenderState() { return m_renderState; }
    const RenderState &getRenderState() const { return m_renderState; }

private:
    void renderGameContent();
    void renderUI();

    void renderTank(entt::entity entity, const Transform &transform, const Tank &tank);
    void renderBullet(const Transform &transform, const Bullet &bullet);
    void renderCollectible(const Transform &transform, const Collectible &collectible);
    void renderDestructible(const Transform &transform, const Destructible &destructible);

    const Game &m_game;
    RenderState m_renderState;
    int m_windowWidth;
    int m_windowHeight;
};

#endif  // RENDERER_H

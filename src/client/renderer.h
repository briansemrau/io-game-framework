#ifndef RENDERER_H
#define RENDERER_H

#include "raylib.h"

#include "game_state.h"

struct RenderState {
    float zoom = 30.0f;
    bool debugDrawEnabled = false;
    bool handbrake = false;
};

class Renderer {
public:
    void render(const GameState& gameState, const RenderState& renderState);

    void setZoom(float zoom);
    float getZoom() const;
    void handleMouseWheel(float delta);

private:
    void renderCar(const Car& car, Vector2 screenPos, float zoom, Color color);
    void renderPlayerCar(const Car& car, Vector2 screenPos, float zoom, bool handbrake);

    float m_zoom = 30.0f;
    static constexpr float MinZoom = 10.0f;
    static constexpr float MaxZoom = 60.0f;
    static constexpr float ZoomDelta = 5.0f;
};

#endif // RENDERER_H

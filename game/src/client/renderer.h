#ifndef RENDERER_H
#define RENDERER_H

#include "game.h"

// Data specific to the current window
// (we do not handle multiple cameras)
struct RenderState {
    bool debugDrawEnabled = false;
    // TODO camera
    // 
};

class Renderer {
public:
    void render(const GameState& gameState);

    RenderState m_renderState;

private:
    // void renderCar(const Car& car, Vector2 screenPos, float zoom, Color color);

    // TODO register renderer for a given entity
};

#endif // RENDERER_H

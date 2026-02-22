#ifndef CLIENT_INSTANCE_H
#define CLIENT_INSTANCE_H

#include "game.h"
#include "network_client.h"
#include "renderer.h"

class ClientInstance {
public:
    void run();

private:
    Game m_game;
    NetworkClient m_networkClient;
    Renderer m_renderer;
    RenderState m_renderState;

    void handleInput();
    void step();
    void render();
};

#endif  // CLIENT_INSTANCE_H

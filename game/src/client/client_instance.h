#ifndef CLIENT_INSTANCE_H
#define CLIENT_INSTANCE_H

#include "game.h"
#include "network_client.h"
#include "renderer.h"

class ClientInstance {
public:
    ClientInstance();

    void run();

private:
    Game m_game;
    NetworkClient m_networkClient;
    Renderer m_renderer;

    // TODO: support running P2P server?
    // NetworkServer *m_networkServer;
    // we would need to disable networkclient

    void handleInput();
    void step();
    void render();
};

#endif  // CLIENT_INSTANCE_H

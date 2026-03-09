#ifndef SERVER_INSTANCE_H
#define SERVER_INSTANCE_H

#include <atomic>

#include "game.h"
#include "network_server.h"

class ServerInstance {
public:
    ServerInstance();

    void run();
    void stop();

private:
    Game m_game;
    NetworkServer m_networkServer;

    std::atomic<bool> m_killFlag{ false };

    void step();
};

#endif  // SERVER_INSTANCE_H

#ifndef GAME_H
#define GAME_H

#include <entt/entt.hpp>
#include <cstdint>
#include <memory>
#include <shared_mutex>

#include "tank_components.h"
#include "types.h"

class GameState {
public:
    GameState() = default;
    GameState(const GameState &) = default;
    GameState(GameState &&) noexcept = default;
    GameState &operator=(const GameState &) = default;
    GameState &operator=(GameState &&) noexcept = default;
    ~GameState() = default;

    uint64_t tickCount{};
    uint32_t server_subobject_id_counter{ 1 };

    entt::registry registry;
};

class Game {
public:
    explicit Game(bool p_isServer = false);
    Game(const Game &) = delete;
    Game(Game &&) noexcept = delete;
    Game &operator=(const Game &) = delete;
    Game &operator=(Game &&) noexcept = delete;
    ~Game() = default;

    void step();

    void queueInput(const InputState &input, PeerID peerId);

    entt::entity spawnTank(b2Vec2 position, float rotation, PeerID peerId);
    entt::entity spawnBullet(b2Vec2 position, float rotation, PeerID peerId);
    entt::entity spawnCollectible(b2Vec2 position);
    entt::entity spawnDestructible(b2Vec2 position);
    void destroyEntity(entt::entity entity);

    void setupInitialGameState();

    float getFixedTimestepDuration() const;

    bool isServer() const;

    GameState &getState();
    const GameState &getState() const;

private:
    bool m_isServer{ false };

    std::unique_ptr<GameState> m_state;
    mutable std::shared_mutex m_stateMutex;

    static constexpr float FIXED_TIMESTEP{ 1.0f / 60.0f };
};

#endif  // GAME_H
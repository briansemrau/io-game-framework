#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <memory>
#include <shared_mutex>
#include <vector>

#include "box2d/box2d.h"

using id_t = uint64_t;

static constexpr uint32_t FixedTimestepsPerSecond = 60;
static constexpr float FixedTimestepDuration = 1.0f / static_cast<float>(FixedTimestepsPerSecond);

class GameState {
public:
    GameState();

    GameState(const GameState &);
    GameState(GameState &&) noexcept;
    GameState &operator=(const GameState &);
    GameState &operator=(GameState &&) noexcept;
    virtual ~GameState();

    uint64_t tickCount{};
    uint32_t server_subobject_id_counter{1};

    uint32_t testData{};

    b2WorldId worldId{};

    // constexpr static auto serialize(auto &archive, auto &self) {
    //     return archive(
    //         self.tickCount,
    //         self.server_subobject_id_counter,
    //         self.testData
    //     );
    // }
};

class Game {
public:
    static constexpr uint32_t FixedTimestepsPerSecond = 60;
    static constexpr float FixedTimestepDuration = 1.0f / static_cast<float>(FixedTimestepsPerSecond);

    static constexpr float ArenaWidth = 40.0f;
    static constexpr float ArenaHeight = 30.0f;
    static constexpr float ArenaMinX = -ArenaWidth / 2.0f;
    static constexpr float ArenaMinY = -ArenaHeight / 2.0f;
    static constexpr float ArenaMaxX = ArenaWidth / 2.0f;
    static constexpr float ArenaMaxY = ArenaHeight / 2.0f;

    Game(bool p_isServer = false);
    Game(const Game &) = delete;
    Game(Game &&) noexcept = delete;
    Game &operator=(const Game &) = delete;
    Game &operator=(Game &&) noexcept = delete;
    virtual ~Game();

    void step();

    void setState(GameState &);
    const GameState &getState() const;

private:
    void createArenaWall(b2Vec2 center, b2Vec2 halfSize);
    void createArena();

    bool m_isServer{false};
    GameState m_state;
    mutable std::shared_mutex m_stateMutex;
};

#endif  // GAME_STATE_H

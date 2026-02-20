#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <memory>
#include <vector>

#include "box2d/box2d.h"
#include "car.h"

static constexpr uint32_t FixedTimestepsPerSecond = 60;
static constexpr float FixedTimestepDuration = 1.0f / static_cast<float>(FixedTimestepsPerSecond);

class GameState {
public:
    static constexpr uint32_t FixedTimestepsPerSecond = 60;
    static constexpr float FixedTimestepDuration = 1.0f / static_cast<float>(FixedTimestepsPerSecond);

    static constexpr float ArenaWidth = 40.0f;
    static constexpr float ArenaHeight = 30.0f;
    static constexpr float ArenaMinX = -ArenaWidth / 2.0f;
    static constexpr float ArenaMinY = -ArenaHeight / 2.0f;
    static constexpr float ArenaMaxX = ArenaWidth / 2.0f;
    static constexpr float ArenaMaxY = ArenaHeight / 2.0f;

    GameState();
    virtual ~GameState();
    GameState(const GameState &);
    GameState(const GameState &&) noexcept;
    GameState &operator=(const GameState &);
    GameState &operator=(const GameState &&) noexcept;

    void reset();
    void step();

    b2WorldId getWorldId() const { return worldId; }

private:
    void createArenaWall(b2Vec2 center, b2Vec2 halfSize);
    void createArena();

    b2WorldId worldId{};
};

#endif  // GAME_STATE_H

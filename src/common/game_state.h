#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "box2d/box2d.h"

#include <vector>
#include <memory>

#include "car.h"

static constexpr uint32_t fixed_timesteps_per_second = 60;
static constexpr float fixed_timestep_duration = 1.0f / static_cast<float>(fixed_timesteps_per_second);

struct Obstacle {
    b2Vec2 position;
    b2Vec2 halfSize;
    float angle;
};

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

    GameState() = default;
    ~GameState();

    GameState(const GameState&) = delete;
    GameState& operator=(const GameState&) = delete;
    GameState(GameState&&) noexcept = default;
    GameState& operator=(GameState&&) noexcept = default;

    void init();
    void shutdown();
    void reset();
    void step();

    [[nodiscard]] b2WorldId getWorldId() const { return worldId; }
    [[nodiscard]] Car& getPlayerCar() { return playerCar; }
    [[nodiscard]] const Car& getPlayerCar() const { return playerCar; }
    [[nodiscard]] const std::vector<Car>& getAICars() const { return aiCars; }
    [[nodiscard]] const std::vector<Obstacle>& getObstacles() const { return obstacles; }

    void addAICar(b2Vec2 position);

private:
    void createArenaWall(b2Vec2 center, b2Vec2 halfSize);
    void createObstacle(b2Vec2 center, b2Vec2 halfSize, float angle = 0.0f);
    void createObstacles();
    void createArena();
    void destroyAllCars();
    void updateAICars();

    b2WorldId worldId{};
    Car playerCar;
    std::vector<Car> aiCars;
    std::vector<Obstacle> obstacles;
    float aiTimer = 0.0f;
};

[[nodiscard]] GameState& getGameState();

void initCommon();
void fixedTimestep();
void resetGame();
[[nodiscard]] b2WorldId getWorldId();
[[nodiscard]] Car& getPlayerCar();
[[nodiscard]] const std::vector<Obstacle>& getObstacles();
[[nodiscard]] const std::vector<Car>& getAICars();
void addAICar(b2Vec2 position);

#endif // GAME_STATE_H

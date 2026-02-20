#ifndef COMMON_MAIN_H
#define COMMON_MAIN_H

#include "box2d/box2d.h"
#include "car.h"
#include <vector>

static constexpr uint32_t fixed_timesteps_per_second = 60;
static constexpr float fixed_timestep_duration = 1.0f / static_cast<float>(fixed_timesteps_per_second);

struct Obstacle
{
    b2Vec2 position;
    b2Vec2 halfSize;
    float angle;
};

void initCommon();
void fixedTimestep();
void resetGame();
[[nodiscard]] b2WorldId getWorldId();
[[nodiscard]] Car& getPlayerCar();
[[nodiscard]] const std::vector<Obstacle>& getObstacles();
[[nodiscard]] const std::vector<Car>& getAICars();
void addAICar(b2Vec2 position);

#endif // COMMON_MAIN_H

#ifndef COMMON_MAIN_H
#define COMMON_MAIN_H

#include "box2d/box2d.h"
#include "car.h"
#include <vector>

struct Obstacle
{
    b2Vec2 position;
    b2Vec2 halfSize;
    float angle;
};

void initCommon();
void stepCommon();
void resetGame();
[[nodiscard]] b2WorldId getWorldId();
[[nodiscard]] Car& getPlayerCar();
[[nodiscard]] const std::vector<Obstacle>& getObstacles();
[[nodiscard]] const std::vector<Car>& getAICars();
void addAICar(b2Vec2 position);

#endif // COMMON_MAIN_H

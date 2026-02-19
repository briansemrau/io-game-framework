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
b2WorldId getWorldId();
Car& getPlayerCar();
const std::vector<Obstacle>& getObstacles();
const std::vector<Car>& getAICars();
void addAICar(b2Vec2 position);

#endif // COMMON_MAIN_H

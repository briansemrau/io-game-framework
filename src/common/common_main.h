#ifndef COMMON_MAIN_H
#define COMMON_MAIN_H

#include "box2d/box2d.h"
#include "car.h"

void initCommon();
void stepCommon();
b2WorldId getWorldId();
Car& getPlayerCar();

#endif // COMMON_MAIN_H

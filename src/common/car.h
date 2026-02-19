#ifndef CAR_H
#define CAR_H

#include "box2d/box2d.h"
#include <cstdint>

struct Car
{
    b2BodyId bodyId;
    float width;
    float height;
    float maxSpeed;
    float acceleration;
    float turnSpeed;
    float friction;
    float driftFactor;
    float health;
    float maxHealth;

    float throttleInput;
    float turnInput;
    bool handbrakeInput;
};

enum class CarCollisionBits : uint64_t
{
    StaticBit = 0x0001,
    CarBit = 0x0002,
};

void createCar(b2WorldId worldId, Car& car, b2Vec2 position);
void destroyCar(b2WorldId worldId, Car& car);
void carSetInput(Car& car, float throttle, float turn, bool handbrake);
void carUpdate(Car& car, float deltaTime);

#endif // CAR_H

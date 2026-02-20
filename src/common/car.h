#ifndef CAR_H
#define CAR_H

#include "box2d/box2d.h"
#include <cstdint>

#include "physics_settings.h"

class Car {
public:
    Car() = default;

    void create(b2WorldId worldId, b2Vec2 position);
    void destroy();
    void setInput(float throttle, float turn, bool handbrake);
    void update(float deltaTime);

    b2BodyId getBodyId() const { return bodyId; }
    b2Vec2 getPosition() const;
    b2Vec2 getVelocity() const;
    float getAngle() const;

    b2BodyId bodyId = {};
    float width = 1.5f;
    float height = 2.5f;
    float maxSpeed = 15.0f;
    float acceleration = 20.0f;
    float turnSpeed = 3.5f;
    float friction = 0.98f;
    float driftFactor = 0.95f;
    float health = 100.0f;
    float maxHealth = 100.0f;

    float throttleInput = 0.0f;
    float turnInput = 0.0f;
    bool handbrakeInput = false;
};

void carSetInput(Car& car, float throttle, float turn, bool handbrake);

#endif // CAR_H

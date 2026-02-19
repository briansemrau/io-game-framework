#include "car.h"
#include "physics_settings.h"
#include <cmath>

void createCar(b2WorldId worldId, Car& car, b2Vec2 position)
{
    car.width = 1.5f;
    car.height = 2.5f;
    car.maxSpeed = 15.0f;
    car.acceleration = 20.0f;
    car.turnSpeed = 3.5f;
    car.friction = 0.98f;
    car.driftFactor = 0.95f;

    car.throttleInput = 0.0f;
    car.turnInput = 0.0f;
    car.handbrakeInput = false;

    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = position;
    bodyDef.rotation = b2MakeRot(0.0f);
    bodyDef.linearDamping = 0.5f;
    bodyDef.angularDamping = 2.0f;

    car.bodyId = b2CreateBody(worldId, &bodyDef);

    b2Polygon polygon = b2MakeBox(car.width / 2.0f, car.height / 2.0f);
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 1.0f;
    b2SurfaceMaterial material = b2DefaultSurfaceMaterial();
    material.friction = 0.3f;
    material.restitution = 0.1f;
    shapeDef.material = material;
    shapeDef.filter.categoryBits = static_cast<uint64_t>(CarCollisionBits::CarBit);
    shapeDef.filter.maskBits = static_cast<uint64_t>(CarCollisionBits::StaticBit);

    b2CreatePolygonShape(car.bodyId, &shapeDef, &polygon);
}

void destroyCar(b2WorldId worldId, Car& car)
{
    (void)worldId;
    if (b2Body_IsValid(car.bodyId))
    {
        b2DestroyBody(car.bodyId);
    }
}

void carSetInput(Car& car, float throttle, float turn, bool handbrake)
{
    car.throttleInput = throttle;
    car.turnInput = turn;
    car.handbrakeInput = handbrake;
}

void carUpdate(Car& car, float deltaTime)
{
    (void)deltaTime;

    b2Transform transform = b2Body_GetTransform(car.bodyId);
    b2Vec2 velocity = b2Body_GetLinearVelocity(car.bodyId);

    float forwardSpeed = b2Dot(velocity, b2RotateVector(transform.q, {0.0f, 1.0f}));

    float throttleForce = car.throttleInput * car.acceleration;
    b2Vec2 forwardDir = b2RotateVector(transform.q, {0.0f, 1.0f});
    b2Vec2 force = b2MulSV(throttleForce, forwardDir);
    b2Body_ApplyForceToCenter(car.bodyId, force, true);

    if (std::abs(forwardSpeed) > 0.5f)
    {
        float turnDirection = forwardSpeed > 0.0f ? 1.0f : -1.0f;
        float turnAmount = car.turnInput * car.turnSpeed * turnDirection;
        
        if (car.handbrakeInput)
        {
            turnAmount *= 1.5f;
        }
        
        b2Body_SetAngularVelocity(car.bodyId, turnAmount);
    }

    b2Vec2 newVelocity = b2Body_GetLinearVelocity(car.bodyId);
    float lateralFriction = car.handbrakeInput ? 0.98f : car.driftFactor;
    b2Vec2 rightDir = b2RotateVector(transform.q, {1.0f, 0.0f});
    float lateralVelocity = b2Dot(newVelocity, rightDir);
    b2Vec2 lateralImpulse = b2MulSV(-lateralVelocity * lateralFriction, rightDir);
    b2Body_ApplyLinearImpulseToCenter(car.bodyId, lateralImpulse, true);

    b2Vec2 forwardDir2 = b2RotateVector(transform.q, {0.0f, 1.0f});
    float currentForwardSpeed = b2Dot(newVelocity, forwardDir2);
    if (std::abs(currentForwardSpeed) > car.maxSpeed)
    {
        float excessSpeed = currentForwardSpeed - (currentForwardSpeed > 0 ? car.maxSpeed : -car.maxSpeed);
        b2Vec2 excessImpulse = b2MulSV(-excessSpeed * 0.5f, forwardDir2);
        b2Body_ApplyLinearImpulseToCenter(car.bodyId, excessImpulse, true);
    }
}

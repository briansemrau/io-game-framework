#include "box2d/box2d.h"

#include <cmath>

#include "car.h"
#include "physics_settings.h"

void Car::create(b2WorldId worldId, b2Vec2 position) {
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = position;
    bodyDef.rotation = b2MakeRot(0.0f);
    bodyDef.linearDamping = 0.5f;
    bodyDef.angularDamping = 2.0f;

    bodyId = b2CreateBody(worldId, &bodyDef);

    b2Polygon polygon = b2MakeBox(width / 2.0f, height / 2.0f);
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 1.0f;
    b2SurfaceMaterial material = b2DefaultSurfaceMaterial();
    material.friction = 0.3f;
    material.restitution = 0.1f;
    shapeDef.material = material;
    shapeDef.filter.categoryBits = static_cast<uint64_t>(CollisionBits::CarBit);
    shapeDef.filter.maskBits = static_cast<uint64_t>(CollisionBits::StaticBit) | static_cast<uint64_t>(CollisionBits::CarBit);

    b2CreatePolygonShape(bodyId, &shapeDef, &polygon);
}

void Car::destroy() {
    if (b2Body_IsValid(bodyId)) {
        b2DestroyBody(bodyId);
    }
}

void Car::setInput(float throttle, float turn, bool handbrake) {
    throttleInput = throttle;
    turnInput = turn;
    handbrakeInput = handbrake;
}

void Car::update(float deltaTime) {
    (void)deltaTime;

    b2Transform transform = b2Body_GetTransform(bodyId);
    b2Vec2 velocity = b2Body_GetLinearVelocity(bodyId);

    float forwardSpeed = b2Dot(velocity, b2RotateVector(transform.q, {0.0f, 1.0f}));

    float throttleForce = throttleInput * acceleration;
    b2Vec2 forwardDir = b2RotateVector(transform.q, {0.0f, 1.0f});
    b2Vec2 force = b2MulSV(throttleForce, forwardDir);
    b2Body_ApplyForceToCenter(bodyId, force, true);

    if (std::abs(forwardSpeed) > 0.5f) {
        float turnDirection = forwardSpeed > 0.0f ? 1.0f : -1.0f;
        float turnAmount = turnInput * turnSpeed * turnDirection;
        
        if (handbrakeInput) {
            turnAmount *= 1.5f;
        }
        
        b2Body_SetAngularVelocity(bodyId, turnAmount);
    }

    b2Vec2 newVelocity = b2Body_GetLinearVelocity(bodyId);
    float lateralFriction = handbrakeInput ? 0.98f : driftFactor;
    b2Vec2 rightDir = b2RotateVector(transform.q, {1.0f, 0.0f});
    float lateralVelocity = b2Dot(newVelocity, rightDir);
    b2Vec2 lateralImpulse = b2MulSV(-lateralVelocity * lateralFriction, rightDir);
    b2Body_ApplyLinearImpulseToCenter(bodyId, lateralImpulse, true);

    b2Vec2 forwardDir2 = b2RotateVector(transform.q, {0.0f, 1.0f});
    float currentForwardSpeed = b2Dot(newVelocity, forwardDir2);
    if (std::abs(currentForwardSpeed) > maxSpeed) {
        float excessSpeed = currentForwardSpeed - (currentForwardSpeed > 0 ? maxSpeed : -maxSpeed);
        b2Vec2 excessImpulse = b2MulSV(-excessSpeed * 0.5f, forwardDir2);
        b2Body_ApplyLinearImpulseToCenter(bodyId, excessImpulse, true);
    }
}

b2Vec2 Car::getPosition() const {
    return b2Body_GetTransform(bodyId).p;
}

b2Vec2 Car::getVelocity() const {
    return b2Body_GetLinearVelocity(bodyId);
}

float Car::getAngle() const {
    return b2Rot_GetAngle(b2Body_GetTransform(bodyId).q);
}

void carSetInput(Car& car, float throttle, float turn, bool handbrake) {
    car.setInput(throttle, turn, handbrake);
}

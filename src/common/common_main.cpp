#include "box2d/box2d.h"
#include <cstddef>

#include "physics_settings.h"

static b2WorldId worldId;

void initCommon() {
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = {0.0f, 9.81f};
    worldId = b2CreateWorld(&worldDef);
    
    b2World_SetPreSolveCallback(worldId, [](b2ShapeId shapeIdA, b2ShapeId shapeIdB, b2Vec2 point, b2Vec2 normal, void*) -> bool {
        return true;
    }, nullptr);
}

void stepCommon() {
    float deltaTime = 1.0f / 60.0f;
    b2World_Step(worldId, deltaTime, 4);
}

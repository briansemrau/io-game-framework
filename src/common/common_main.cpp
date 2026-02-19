#include "common_main.h"
#include "physics_settings.h"
#include <cstddef>

static b2WorldId worldId;
static Car playerCar;
static b2Vec2 arenaMin = {-20.0f, -15.0f};
static b2Vec2 arenaMax = {20.0f, 15.0f};

void createArenaWall(b2Vec2 center, b2Vec2 halfSize)
{
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_staticBody;
    bodyDef.position = center;
    bodyDef.rotation = b2MakeRot(0.0f);
    b2BodyId wallId = b2CreateBody(worldId, &bodyDef);

    b2Polygon polygon = b2MakeBox(halfSize.x, halfSize.y);
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    b2SurfaceMaterial material = b2DefaultSurfaceMaterial();
    material.friction = 0.5f;
    material.restitution = 0.2f;
    shapeDef.material = material;
    shapeDef.filter.categoryBits = static_cast<uint64_t>(CarCollisionBits::StaticBit);
    shapeDef.filter.maskBits = static_cast<uint64_t>(CarCollisionBits::CarBit);

    b2CreatePolygonShape(wallId, &shapeDef, &polygon);
}

void createArena()
{
    float wallThickness = 1.0f;
    
    createArenaWall({0.0f, arenaMax.y + wallThickness}, {arenaMax.x + wallThickness, wallThickness});
    createArenaWall({0.0f, arenaMin.y - wallThickness}, {arenaMax.x + wallThickness, wallThickness});
    createArenaWall({arenaMax.x + wallThickness, 0.0f}, {wallThickness, arenaMax.y + wallThickness});
    createArenaWall({arenaMin.x - wallThickness, 0.0f}, {wallThickness, arenaMax.y + wallThickness});
}

void initCommon() {
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = {0.0f, 0.0f};
    worldId = b2CreateWorld(&worldDef);
    
    createArena();
    createCar(worldId, playerCar, {0.0f, 0.0f});
    
    b2World_SetPreSolveCallback(worldId, [](b2ShapeId shapeIdA, b2ShapeId shapeIdB, b2Vec2 point, b2Vec2 normal, void*) -> bool {
        return true;
    }, nullptr);
}

void stepCommon() {
    float deltaTime = 1.0f / 60.0f;
    carUpdate(playerCar, deltaTime);
    b2World_Step(worldId, deltaTime, 4);
}

b2WorldId getWorldId() {
    return worldId;
}

Car& getPlayerCar() {
    return playerCar;
}

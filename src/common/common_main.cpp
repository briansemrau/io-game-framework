#include "common_main.h"
#include "physics_settings.h"
#include <cstddef>
#include <vector>
#include <cmath>

static b2WorldId worldId;
static Car playerCar;
static std::vector<Car> aiCars;
static b2Vec2 arenaMin = {-20.0f, -15.0f};
static b2Vec2 arenaMax = {20.0f, 15.0f};
static std::vector<Obstacle> obstacles;

void updateAICars();

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

void createObstacle(b2Vec2 center, b2Vec2 halfSize, float angle = 0.0f)
{
    obstacles.push_back({center, halfSize, angle});

    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_staticBody;
    bodyDef.position = center;
    bodyDef.rotation = b2MakeRot(angle);
    b2BodyId obstacleId = b2CreateBody(worldId, &bodyDef);

    b2Polygon polygon = b2MakeBox(halfSize.x, halfSize.y);
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    b2SurfaceMaterial material = b2DefaultSurfaceMaterial();
    material.friction = 0.7f;
    material.restitution = 0.3f;
    shapeDef.material = material;
    shapeDef.filter.categoryBits = static_cast<uint64_t>(CarCollisionBits::StaticBit);
    shapeDef.filter.maskBits = static_cast<uint64_t>(CarCollisionBits::CarBit);

    b2CreatePolygonShape(obstacleId, &shapeDef, &polygon);
}

void createObstacles()
{
    createObstacle({5.0f, 5.0f}, {1.5f, 1.5f});
    createObstacle({-6.0f, -4.0f}, {1.0f, 3.0f});
    createObstacle({8.0f, -5.0f}, {2.0f, 1.0f}, 0.785f);
    createObstacle({-3.0f, 8.0f}, {0.8f, 0.8f});
    createObstacle({10.0f, 0.0f}, {1.0f, 4.0f});
    createObstacle({-10.0f, 2.0f}, {1.5f, 2.0f}, -0.5f);
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
    createObstacles();
    createCar(worldId, playerCar, {0.0f, 0.0f});
    
    addAICar({-5.0f, -5.0f});
    addAICar({5.0f, 5.0f});
    addAICar({-5.0f, 5.0f});
    
    b2World_SetPreSolveCallback(worldId, [](b2ShapeId shapeIdA, b2ShapeId shapeIdB, b2Vec2 point, b2Vec2 normal, void*) -> bool {
        return true;
    }, nullptr);
}

void stepCommon() {
    float deltaTime = 1.0f / 60.0f;
    carUpdate(playerCar, deltaTime);
    updateAICars();
    b2World_Step(worldId, deltaTime, 4);
}

b2WorldId getWorldId() {
    return worldId;
}

Car& getPlayerCar() {
    return playerCar;
}

const std::vector<Obstacle>& getObstacles() {
    return obstacles;
}

void addAICar(b2Vec2 position)
{
    aiCars.push_back(Car{});
    createCar(worldId, aiCars.back(), position);
}

const std::vector<Car>& getAICars() {
    return aiCars;
}

void updateAICars()
{
    static float time = 0.0f;
    time += 1.0f / 60.0f;
    
    int index = 0;
    for (auto& aiCar : aiCars)
    {
        b2Vec2 carPos = b2Body_GetTransform(aiCar.bodyId).p;
        
        float targetX = carPos.x + sinf(time * 0.5f + (float)index) * 5.0f;
        float targetY = carPos.y + cosf(time * 0.3f + (float)index) * 5.0f;
        
        b2Vec2 toTarget = {targetX - carPos.x, targetY - carPos.y};
        float targetAngle = atan2f(toTarget.x, toTarget.y);
        
        b2Transform transform = b2Body_GetTransform(aiCar.bodyId);
        float currentAngle = b2Rot_GetAngle(transform.q);
        
        float angleDiff = targetAngle - currentAngle;
        while (angleDiff > 3.14159f) angleDiff -= 2.0f * 3.14159f;
        while (angleDiff < -3.14159f) angleDiff += 2.0f * 3.14159f;
        
        float turnInput = angleDiff * 2.0f;
        if (turnInput > 1.0f) turnInput = 1.0f;
        if (turnInput < -1.0f) turnInput = -1.0f;
        
        carSetInput(aiCar, 0.8f, turnInput, false);
        index++;
    }
}

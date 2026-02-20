#include "box2d/box2d.h"

#include <cmath>
#include <cstddef>
#include <numbers>
#include <vector>

#include "common_main.h"
#include "physics_settings.h"

static constexpr float DeltaTime = 1.0f / 60.0f;
static constexpr float ArenaWidth = 40.0f;
static constexpr float ArenaHeight = 30.0f;
static constexpr float ArenaMinX = -ArenaWidth / 2.0f;
static constexpr float ArenaMinY = -ArenaHeight / 2.0f;
static constexpr float ArenaMaxX = ArenaWidth / 2.0f;
static constexpr float ArenaMaxY = ArenaHeight / 2.0f;

static b2WorldId worldId;
static Car playerCar;
static std::vector<Car> aiCars;
static b2Vec2 arenaMin = {ArenaMinX, ArenaMinY};
static b2Vec2 arenaMax = {ArenaMaxX, ArenaMaxY};
static std::vector<Obstacle> obstacles;

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
    shapeDef.filter.categoryBits = static_cast<uint64_t>(CollisionBits::StaticBit);
    shapeDef.filter.maskBits = static_cast<uint64_t>(CollisionBits::CarBit);

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
    shapeDef.filter.categoryBits = static_cast<uint64_t>(CollisionBits::StaticBit);
    shapeDef.filter.maskBits = static_cast<uint64_t>(CollisionBits::CarBit);

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
    addAICar({8.0f, -3.0f});
    addAICar({-8.0f, 3.0f});
    
    b2World_SetPreSolveCallback(worldId, [](b2ShapeId shapeIdA, b2ShapeId shapeIdB, b2Vec2 point, b2Vec2 normal, void*) -> bool {
        return true;
    }, nullptr);
}

void updateAICars()
{
    static float time = 0.0f;
    time += DeltaTime;
    
    b2Vec2 playerPos = b2Body_GetTransform(playerCar.bodyId).p;
    
    int index = 0;
    for (auto& aiCar : aiCars)
    {
        b2Vec2 carPos = b2Body_GetTransform(aiCar.bodyId).p;
        
        float followPlayer = (index % 2 == 0) ? 0.7f : 0.0f;
        float targetX, targetY;
        
        if (followPlayer > 0.0f)
        {
            targetX = playerPos.x + sinf(time * 0.5f + (float)index * 1.5f) * 3.0f;
            targetY = playerPos.y + cosf(time * 0.3f + (float)index * 1.5f) * 3.0f;
        }
        else
        {
            targetX = carPos.x + sinf(time * 0.5f + (float)index) * 8.0f;
            targetY = carPos.y + cosf(time * 0.3f + (float)index) * 8.0f;
        }
        
        b2Vec2 toTarget = {targetX - carPos.x, targetY - carPos.y};
        float targetAngle = atan2f(toTarget.x, toTarget.y);
        
        b2Transform transform = b2Body_GetTransform(aiCar.bodyId);
        float currentAngle = b2Rot_GetAngle(transform.q);
        
        float angleDiff = targetAngle - currentAngle;
        while (angleDiff > std::numbers::pi) angleDiff -= 2.0f * std::numbers::pi;
        while (angleDiff < -std::numbers::pi) angleDiff += 2.0f * std::numbers::pi;
        
        float turnInput = angleDiff * 2.0f;
        if (turnInput > 1.0f) turnInput = 1.0f;
        if (turnInput < -1.0f) turnInput = -1.0f;
        
        float throttleInput = 0.6f + followPlayer * 0.3f;
        carSetInput(aiCar, throttleInput, turnInput, false);
        index++;
    }
}

void fixedTimestep() {
    float deltaTime = DeltaTime;
    carUpdate(playerCar, deltaTime);
    updateAICars();
    b2World_Step(worldId, deltaTime, 4);
}

void resetGame()
{
    for (auto& aiCar : aiCars)
    {
        if (b2Body_IsValid(aiCar.bodyId))
        {
            b2DestroyBody(aiCar.bodyId);
        }
    }
    aiCars.clear();

    if (b2Body_IsValid(playerCar.bodyId))
    {
        b2DestroyBody(playerCar.bodyId);
    }

    createCar(worldId, playerCar, {0.0f, 0.0f});

    addAICar({-5.0f, -5.0f});
    addAICar({5.0f, 5.0f});
    addAICar({-5.0f, 5.0f});
    addAICar({8.0f, -3.0f});
    addAICar({-8.0f, 3.0f});
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

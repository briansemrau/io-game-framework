#include "box2d/box2d.h"

#include <cmath>
#include <numbers>

#include "game_state.h"
#include "physics_settings.h"

GameState::~GameState() {
    shutdown();
}

void GameState::init() {
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = {0.0f, 0.0f};
    worldId = b2CreateWorld(&worldDef);
    
    createArena();
    createObstacles();
    playerCar.create(worldId, {0.0f, 0.0f});
    
    addAICar({-5.0f, -5.0f});
    addAICar({5.0f, 5.0f});
    addAICar({-5.0f, 5.0f});
    addAICar({8.0f, -3.0f});
    addAICar({-8.0f, 3.0f});
    
    b2World_SetPreSolveCallback(worldId, [](b2ShapeId shapeIdA, b2ShapeId shapeIdB, b2Vec2 point, b2Vec2 normal, void*) -> bool {
        return true;
    }, nullptr);
}

void GameState::shutdown() {
    destroyAllCars();
    if (b2World_IsValid(worldId)) {
        b2DestroyWorld(worldId);
    }
    worldId = {};
}

void GameState::reset() {
    destroyAllCars();
    playerCar.create(worldId, {0.0f, 0.0f});

    addAICar({-5.0f, -5.0f});
    addAICar({5.0f, 5.0f});
    addAICar({-5.0f, 5.0f});
    addAICar({8.0f, -3.0f});
    addAICar({-8.0f, 3.0f});
}

void GameState::step() {
    const float deltaTime = FixedTimestepDuration;
    playerCar.update(deltaTime);
    aiSystem.update(playerCar, aiCars, deltaTime);
    b2World_Step(worldId, deltaTime, 4);
}

void GameState::addAICar(b2Vec2 position) {
    aiCars.emplace_back();
    aiCars.back().create(worldId, position);
}

void GameState::destroyAllCars() {
    for (auto& car : aiCars) {
        car.destroy();
    }
    aiCars.clear();
    playerCar.destroy();
}

void GameState::createArenaWall(b2Vec2 center, b2Vec2 halfSize) {
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

void GameState::createObstacle(b2Vec2 center, b2Vec2 halfSize, float angle) {
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

void GameState::createObstacles() {
    createObstacle({5.0f, 5.0f}, {1.5f, 1.5f});
    createObstacle({-6.0f, -4.0f}, {1.0f, 3.0f});
    createObstacle({8.0f, -5.0f}, {2.0f, 1.0f}, 0.785f);
    createObstacle({-3.0f, 8.0f}, {0.8f, 0.8f});
    createObstacle({10.0f, 0.0f}, {1.0f, 4.0f});
    createObstacle({-10.0f, 2.0f}, {1.5f, 2.0f}, -0.5f);
}

void GameState::createArena() {
    const float wallThickness = 1.0f;
    const b2Vec2 min{ArenaMinX, ArenaMinY};
    const b2Vec2 max{ArenaMaxX, ArenaMaxY};
    
    createArenaWall({0.0f, max.y + wallThickness}, {max.x + wallThickness, wallThickness});
    createArenaWall({0.0f, min.y - wallThickness}, {max.x + wallThickness, wallThickness});
    createArenaWall({max.x + wallThickness, 0.0f}, {wallThickness, max.y + wallThickness});
    createArenaWall({min.x - wallThickness, 0.0f}, {wallThickness, max.y + wallThickness});
}

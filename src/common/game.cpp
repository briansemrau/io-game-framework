#include "game.h"

#include <cmath>
#include <numbers>

#include "box2d/box2d.h"
#include "physics_settings.h"

GameState::GameState() {
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = {0.0f, 0.0f};
    worldId = b2CreateWorld(&worldDef);
}

GameState::GameState(const GameState &other) : tickCount{other.tickCount}, server_subobject_id_counter{other.server_subobject_id_counter}, testData{other.testData} {
    // TODO: Properly clone world state if needed for rollback
    // worldId = ...;
}

GameState::GameState(GameState &&other) noexcept : worldId{std::exchange(other.worldId, {})} {}

GameState &GameState::operator=(const GameState &other) {
    if (this != &other) {
        worldId = other.worldId;
    }
    return *this;
    // TODO: Properly clone world state if needed for rollback
}

GameState &GameState::operator=(GameState &&other) noexcept {
    if (this != &other) {
        worldId = std::exchange(other.worldId, {});
    }
    return *this;
}

GameState::~GameState() {
    if (b2World_IsValid(worldId)) {
        b2DestroyWorld(worldId);
    }
    worldId = {};
}

void Game::step() {
    const float deltaTime = FixedTimestepDuration;
    b2World_Step(m_state.worldId, deltaTime, 4);
}

void Game::setState(GameState &p_state) {
    m_state = p_state;
}

const GameState &Game::getState() const {
    return m_state;
}

void Game::createArenaWall(b2Vec2 center, b2Vec2 halfSize) {
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_staticBody;
    bodyDef.position = center;
    bodyDef.rotation = b2MakeRot(0.0f);
    b2BodyId wallId = b2CreateBody(m_state.worldId, &bodyDef);

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

void Game::createArena() {
    const float wallThickness = 1.0f;
    const b2Vec2 min{ArenaMinX, ArenaMinY};
    const b2Vec2 max{ArenaMaxX, ArenaMaxY};

    createArenaWall({0.0f, max.y + wallThickness}, {max.x + wallThickness, wallThickness});
    createArenaWall({0.0f, min.y - wallThickness}, {max.x + wallThickness, wallThickness});
    createArenaWall({max.x + wallThickness, 0.0f}, {wallThickness, max.y + wallThickness});
    createArenaWall({min.x - wallThickness, 0.0f}, {wallThickness, max.y + wallThickness});
}

Game::Game() {
    createArena();

    // TODO slot/signals
    b2World_SetPreSolveCallback(m_state.worldId, [](b2ShapeId shapeIdA, b2ShapeId shapeIdB, b2Vec2 point, b2Vec2 normal, void *) -> bool { return true; }, nullptr);
}

Game::~Game() {}

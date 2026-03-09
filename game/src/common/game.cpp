#include "game.h"

#include "tank_systems.h"

Game::Game(bool p_isServer) : m_isServer(p_isServer), m_state(std::make_unique<GameState>()) {
    if (m_isServer) {
        setupInitialGameState();
    }
}

void Game::step() {
    std::unique_lock lock(m_stateMutex);
    m_state->tickCount++;
    MovementSystem::update(m_state->registry, FIXED_TIMESTEP);
    ShootSystem::update(m_state->registry, FIXED_TIMESTEP, m_state->tickCount);
    CollisionSystem::check(m_state->registry);
}

void Game::queueInput(const InputState &input, PeerID peerId) {
    std::unique_lock lock(m_stateMutex);

    InputSystem::apply(m_state->registry, peerId, input);

    if (input.shoot) {
        ShootSystem::process(m_state->registry, m_state->tickCount, InputEvent{ m_state->tickCount, peerId, input });
    }
}

entt::entity Game::spawnTank(b2Vec2 position, float rotation, PeerID peerId) { return SpawnSystem::process(m_state->registry, ENTITY_TYPE_PLAYER, position, rotation, peerId); }

entt::entity Game::spawnBullet(b2Vec2 position, float rotation, PeerID peerId) { return SpawnSystem::process(m_state->registry, ENTITY_TYPE_BULLET, position, rotation, peerId); }

entt::entity Game::spawnCollectible(b2Vec2 position) {
    auto &state = static_cast<GameState &>(*m_state);
    return SpawnSystem::process(m_state->registry, ENTITY_TYPE_COLLECTIBLE, position, 0.0f, 0);
}

entt::entity Game::spawnDestructible(b2Vec2 position) { return SpawnSystem::process(m_state->registry, ENTITY_TYPE_DESTRUCTIBLE, position, 0.0f, 0); }

void Game::destroyEntity(entt::entity entity) {
    std::unique_lock lock(m_stateMutex);
    if (m_state->registry.valid(entity)) {
        m_state->registry.destroy(entity);
    }
}

void Game::setupInitialGameState() {
    spawnCollectible(b2Vec2(10.0f, 10.0f));
    spawnCollectible(b2Vec2(-10.0f, 15.0f));
    spawnCollectible(b2Vec2(15.0f, -10.0f));

    spawnDestructible(b2Vec2(20.0f, 0.0f));
    spawnDestructible(b2Vec2(-20.0f, 5.0f));
    spawnDestructible(b2Vec2(0.0f, -25.0f));
}

float Game::getFixedTimestepDuration() const { return FIXED_TIMESTEP; }

bool Game::isServer() const { return m_isServer; }

GameState &Game::getState() { return *m_state; }

const GameState &Game::getState() const { return *m_state; }

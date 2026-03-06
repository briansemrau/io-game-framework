#include "game.h"

#include <mutex>
#include <shared_mutex>

GameState::GameState() {}

GameState::GameState(const GameState &other) : tickCount{ other.tickCount }, server_subobject_id_counter{ other.server_subobject_id_counter }, testData{ other.testData } {}

GameState::GameState(GameState &&other) noexcept {}

GameState &GameState::operator=(const GameState &other) { return *this; }

GameState &GameState::operator=(GameState &&other) noexcept { return *this; }

GameState::~GameState() {}

void Game::step() {
    std::unique_lock lock(m_stateMutex);
    // TODO
}

void Game::setState(GameState &p_state) {
    std::unique_lock lock(m_stateMutex);
    m_state = p_state;
}

const GameState &Game::getState() const {
    std::shared_lock lock(m_stateMutex);
    return m_state;
}

Game::Game(bool p_isServer) : m_isServer(p_isServer) {}

Game::~Game() {}

#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <memory>
#include <shared_mutex>
#include <vector>

class GameState {
public:
    GameState();

    GameState(const GameState &);
    GameState(GameState &&) noexcept;
    GameState &operator=(const GameState &);
    GameState &operator=(GameState &&) noexcept;
    virtual ~GameState();

    uint64_t tickCount{};
    uint32_t server_subobject_id_counter{ 1 };

    uint32_t testData{};

    // constexpr static auto serialize(auto &archive, auto &self) {
    //     return archive(
    //         self.tickCount,
    //         self.server_subobject_id_counter,
    //         self.testData
    //     );
    // }
};

class Game {
public:
    Game(bool p_isServer = false);
    Game(const Game &) = delete;
    Game(Game &&) noexcept = delete;
    Game &operator=(const Game &) = delete;
    Game &operator=(Game &&) noexcept = delete;
    virtual ~Game();

    void step();

    void setState(GameState &);
    const GameState &getState() const;

private:
    bool m_isServer{ false };
    GameState m_state;
    mutable std::shared_mutex m_stateMutex;
};

#endif  // GAME_STATE_H

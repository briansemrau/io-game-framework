#pragma once

#include <box2d/box2d.h>

#include <cstdint>
#include <entt/entt.hpp>

#include "types.h"

// TODO registration to populate these IDs?
inline constexpr uint8_t ENTITY_TYPE_PLAYER = 1;
inline constexpr uint8_t ENTITY_TYPE_BULLET = 2;
inline constexpr uint8_t ENTITY_TYPE_COLLECTIBLE = 3;
inline constexpr uint8_t ENTITY_TYPE_DESTRUCTIBLE = 4;

struct Transform {
    b2Vec2 position{ 0.0f, 0.0f };
    float rotation{ 0.0f };
};

struct Velocity {
    b2Vec2 linear{ 0.0f, 0.0f };
    float angular{ 0.0f };
};

struct Tank {
    float maxSpeed{ 100.0f };
    float acceleration{ 20.0f };
    float turnSpeed{ 2.0f };
    float friction{ 0.98f };
    bool isPlayer{ false };
    PeerID peerId{ 0 };
};

struct Bullet {
    float speed{ 400.0f };
    float lifetime{ 2.0f };
    PeerID ownerPeerId{ 0 };
    uint64_t spawnTick{ 0 };
};

struct Collectible {
    float value{ 10.0f };
    float radius{ 1.0f };
    bool active{ true };
};

struct Destructible {
    float health{ 100.0f };
    float maxHealth{ 100.0f };
    float radius{ 2.0f };
    bool active{ true };
};

struct InputState {
    float throttle{ 0.0f };
    float turn{ 0.0f };
    bool shoot{ false };
};

struct InputEvent {
    uint64_t tick;
    PeerID peerId;
    InputState input;
};

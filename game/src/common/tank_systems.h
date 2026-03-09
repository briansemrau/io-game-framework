#pragma once

#include <entt/entt.hpp>
#include <vector>

#include "tank_components.h"

// Movement system - applies velocity based on input
class MovementSystem {
public:
    static void update(entt::registry &registry, float deltaTime) {
        registry.view<Transform, Velocity, Tank>().each([deltaTime](Transform &transform, Velocity &velocity, Tank &tank) {
            // Apply friction first
            velocity.linear.x *= tank.friction;
            velocity.linear.y *= tank.friction;
            velocity.angular *= tank.friction;

            // Clamp speed to maxSpeed
            float speed = b2Length(velocity.linear);
            if (speed > tank.maxSpeed) {
                velocity.linear *= (tank.maxSpeed / speed);
            }

            // Apply velocity to position
            transform.position.x += velocity.linear.x * deltaTime;
            transform.position.y += velocity.linear.y * deltaTime;
            transform.rotation += velocity.angular * deltaTime;
        });
    }
};

// Input system - applies input to tank velocity
class InputSystem {
public:
    static void apply(entt::registry &registry, uint32_t peerId, const InputState &input) {
        registry.view<Transform, Velocity, Tank>().each([peerId, &input](Transform &transform, Velocity &velocity, Tank &tank) {
            if (!tank.isPlayer || tank.peerId != peerId) return;

            // Apply throttle (forward/backward relative to tank direction)
            float cosRot = cosf(transform.rotation);
            float sinRot = sinf(transform.rotation);

            velocity.linear.x += (cosRot * input.throttle) * tank.acceleration;
            velocity.linear.y += (sinRot * input.throttle) * tank.acceleration;

            // Apply turn (angular velocity)
            velocity.angular = input.turn * tank.turnSpeed;
        });
    }
};

// Shoot system - handles shooting and bullet movement
class ShootSystem {
public:
    static void process(entt::registry &registry, uint64_t tick, const InputEvent &inputEvent) {
        if (!inputEvent.input.shoot) return;

        registry.view<Transform, Tank>().each([tick, &inputEvent, &registry](Transform &transform, Tank &tank) {
            if (!tank.isPlayer || tank.peerId != inputEvent.peerId) return;

            // Spawn bullet
            entt::entity bulletId = registry.create();
            float cosRot = cosf(transform.rotation);
            float sinRot = sinf(transform.rotation);
            registry.emplace<Transform>(bulletId, Transform{ { transform.position.x + cosRot * 3.0f, transform.position.y + sinRot * 3.0f }, transform.rotation });
            registry.emplace<Velocity>(bulletId, Velocity{ { cosRot * 400.0f, sinRot * 400.0f }, 0.0f });
            registry.emplace<Bullet>(bulletId, Bullet{ 400.0f, 2.0f, inputEvent.peerId, tick });
        });
    }

    static void update(entt::registry &registry, float deltaTime, uint64_t currentTick) {
        // Collect entities to remove
        std::vector<entt::entity> toRemove;

        // Update bullet positions and mark expired bullets for removal
        registry.view<entt::entity, Transform, Velocity, Bullet>().each(
            [deltaTime, currentTick, &toRemove](entt::entity entity, Transform &transform, Velocity &velocity, Bullet &bullet) {
                transform.position.x += velocity.linear.x * deltaTime;
                transform.position.y += velocity.linear.y * deltaTime;

                // Check if bullet has expired based on spawn tick
                if (currentTick - bullet.spawnTick >= static_cast<uint64_t>(bullet.lifetime * 60.0f)) {
                    toRemove.push_back(entity);
                }
            });

        // Remove expired bullets
        for (const auto &entity : toRemove) {
            registry.destroy(entity);
        }
    }
};

// Collision system - handles entity collisions
class CollisionSystem {
public:
    static void check(entt::registry &registry) {
        std::vector<entt::entity> bulletsToRemove;

        // Bullet vs Destructible
        registry.view<entt::entity, Transform, Bullet>().each([&registry, &bulletsToRemove](entt::entity bulletEntity, Transform &bulletTransform, Bullet &bullet) {
            registry.view<Transform, Destructible>().each([&bulletEntity, &bulletTransform, &registry, &bulletsToRemove](Transform &destTransform, Destructible &destructible) {
                if (!destructible.active) return;

                float dist = b2Distance(bulletTransform.position, destTransform.position);
                if (dist < destructible.radius + 0.5f) {
                    destructible.health -= 25.0f;
                    if (destructible.health <= 0) {
                        destructible.active = false;
                    }
                    bulletsToRemove.push_back(bulletEntity);
                }
            });
        });

        // Bullet vs Tank
        registry.view<entt::entity, Transform, Bullet>().each([&registry, &bulletsToRemove](entt::entity bulletEntity, Transform &bulletTransform, Bullet &bullet) {
            registry.view<Transform, Tank>().each([&bulletEntity, &bulletTransform, &bullet, &bulletsToRemove](Transform &tankTransform, Tank &tank) {
                if (bullet.ownerPeerId == tank.peerId) return;

                float dist = b2Distance(bulletTransform.position, tankTransform.position);
                if (dist < 1.5f + 0.5f) {
                    bulletsToRemove.push_back(bulletEntity);
                }
            });
        });

        // Tank vs Collectible
        registry.view<Transform, Tank>().each([&registry](Transform &tankTransform, Tank &tank) {
            registry.view<Transform, Collectible>().each([&tankTransform, &registry](Transform &collTransform, Collectible &collectible) {
                if (!collectible.active) return;

                float dist = b2Distance(tankTransform.position, collTransform.position);
                if (dist < collectible.radius + 1.5f) {
                    collectible.active = false;
                }
            });
        });

        // Remove collided bullets
        for (const auto &entity : bulletsToRemove) {
            registry.destroy(entity);
        }
    }
};

// Spawn system - creates entities from spawn events
class SpawnSystem {
public:
    static entt::entity process(entt::registry &registry, uint8_t entityType, b2Vec2 position, float rotation, uint32_t peerId) {
        entt::entity entity = registry.create();

        switch (entityType) {
            case ENTITY_TYPE_PLAYER:
                registry.emplace<Transform>(entity, Transform{ position, rotation });
                registry.emplace<Velocity>(entity);
                registry.emplace<Tank>(entity, Tank{ 100.0f, 20.0f, 2.0f, 0.98f, true, peerId });
                break;

            case ENTITY_TYPE_BULLET:
                registry.emplace<Transform>(entity, Transform{ position, rotation });
                registry.emplace<Velocity>(entity);
                registry.emplace<Bullet>(entity, Bullet{ 400.0f, 2.0f, peerId, 0 });
                break;

            case ENTITY_TYPE_COLLECTIBLE:
                registry.emplace<Transform>(entity, Transform{ position, 0.0f });
                registry.emplace<Collectible>(entity);
                break;

            case ENTITY_TYPE_DESTRUCTIBLE:
                registry.emplace<Transform>(entity, Transform{ position, 0.0f });
                registry.emplace<Destructible>(entity);
                break;
        }

        return entity;
    }
};
#ifndef AI_SYSTEM_H
#define AI_SYSTEM_H

#include <vector>

class Car;

class AISystem {
public:
    static constexpr float PatrolXFrequency = 0.5f;
    static constexpr float PatrolYFrequency = 0.3f;
    static constexpr float PatrolRadius = 3.0f;
    static constexpr float WanderRadius = 8.0f;
    static constexpr float TurnScale = 2.0f;
    static constexpr float BaseThrottle = 0.6f;
    static constexpr float FollowPlayerThrottleBoost = 0.3f;
    static constexpr float FollowPlayerChance = 0.7f;

    AISystem() = default;

    void update(Car& playerCar, std::vector<Car>& aiCars, float deltaTime);

private:
    float aiTimer = 0.0f;
};

#endif // AI_SYSTEM_H

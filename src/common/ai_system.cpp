#include "ai_system.h"

#include <cmath>
#include <numbers>

#include "box2d/box2d.h"
#include "car.h"

void AISystem::update(Car& playerCar, std::vector<Car>& aiCars, float deltaTime) {
    aiTimer += deltaTime;
    
    const b2Vec2 playerPos = playerCar.getPosition();
    
    int index = 0;
    for (auto& aiCar : aiCars) {
        const b2Vec2 carPos = aiCar.getPosition();
        
        const float followPlayer = (index % 2 == 0) ? FollowPlayerChance : 0.0f;
        float targetX;
        float targetY;
        
        if (followPlayer > 0.0f) {
            targetX = playerPos.x + std::sinf(aiTimer * PatrolXFrequency + static_cast<float>(index) * 1.5f) * PatrolRadius;
            targetY = playerPos.y + std::cosf(aiTimer * PatrolYFrequency + static_cast<float>(index) * 1.5f) * PatrolRadius;
        } else {
            targetX = carPos.x + std::sinf(aiTimer * PatrolXFrequency + static_cast<float>(index)) * WanderRadius;
            targetY = carPos.y + std::cosf(aiTimer * PatrolYFrequency + static_cast<float>(index)) * WanderRadius;
        }
        
        b2Vec2 toTarget = {targetX - carPos.x, targetY - carPos.y};
        const float targetAngle = std::atan2f(toTarget.x, toTarget.y);
        
        const float currentAngle = aiCar.getAngle();
        
        float angleDiff = targetAngle - currentAngle;
        while (angleDiff > std::numbers::pi) angleDiff -= 2.0f * std::numbers::pi;
        while (angleDiff < -std::numbers::pi) angleDiff += 2.0f * std::numbers::pi;
        
        float turnInput = angleDiff * TurnScale;
        if (turnInput > 1.0f) turnInput = 1.0f;
        if (turnInput < -1.0f) turnInput = -1.0f;
        
        const float throttleInput = BaseThrottle + followPlayer * FollowPlayerThrottleBoost;
        aiCar.setInput(throttleInput, turnInput, false);
        index++;
    }
}

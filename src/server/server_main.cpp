#include "common_main.h"

#include <chrono>
#include <conio.h>
#include <iostream>
#include <thread>

int main() {
    initCommon();

    constexpr float TargetTimestep = 1.0f / 60.0f;
    auto lastTime = std::chrono::steady_clock::now();

    while (true) {
        auto currentTime = std::chrono::steady_clock::now();
        std::chrono::duration<float, std::ratio<1>> elapsed = currentTime - lastTime;
        lastTime = currentTime;

        float deltaTime = elapsed.count();

        fixedTimestep();

        float stepDuration = std::chrono::duration<float>(std::chrono::steady_clock::now() - currentTime).count();
        float sleepTime = TargetTimestep - stepDuration;
        if (sleepTime > 0.0f) {
            std::this_thread::sleep_for(std::chrono::duration<float>(sleepTime));
        }
    }
    
    return 0;
}

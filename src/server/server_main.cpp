#include "game.h"

#include <chrono>
#include <iostream>
#include <thread>
#include <deque>

#include <numeric>
#include <algorithm>

using Seconds = std::chrono::duration<float, std::ratio<1>>;

int main() {
    GameState game_state;
    game_state.init();

    std::deque<Seconds> stepDurationRecord{};
    float timescale = 1.0; // For slowing down under heavy server load

    // TODO add network thread
    // Will copy game state to network thread to process what to send to each client
    // Network thread will only ever carry one game state. Main thread will only queue state if network thread is waiting and network timestep (0.1s?) is elapsed.

    auto previousTime = std::chrono::steady_clock::now();
    auto remainingTime = Seconds::zero();
    while (true) {
        // The sands of time
        const auto currentTime = std::chrono::steady_clock::now();
        const Seconds elapsed = std::min(Seconds(2), Seconds(currentTime - previousTime)); // don't handle major time skips
        previousTime = currentTime;
        remainingTime += elapsed;

        // Orchestration
        // TODO brian: check if we are commanded to stop from a command server?
        // this will take some thought...
        // if (should_stop) {
        //     break;
        // }

        // Do statistics
        stepDurationRecord.push_back(elapsed);
        if (stepDurationRecord.size() > FixedTimestepsPerSecond * 3) {
            stepDurationRecord.pop_front();
        }
        const auto meanStepDuration = std::accumulate(stepDurationRecord.begin(), stepDurationRecord.end(), Seconds::zero()).count() / static_cast<float>(stepDurationRecord.size());
        const auto targetTimescale = 0.9f * meanStepDuration / FixedTimestepDuration; // go slower than the mean to catch up

        // Slow down timestep if we're behind
        if (remainingTime.count() > FixedTimestepDuration) {
            // Lerp timescale from 1.0 to target depending on how far behind we are.
            // Using a rolling window average should keep timescale smooth
            static constexpr auto ramp_time = FixedTimestepDuration * 3;
            float alpha = std::clamp((remainingTime.count() - FixedTimestepDuration) / ramp_time, 0.0f, 1.0f);
            timescale = targetTimescale * alpha + 1.0f * (1.0f - alpha);
        }

        const auto current_timestep = Seconds(FixedTimestepDuration) * timescale;

        // Do work
        if (remainingTime >= current_timestep) {
            game_state.step();
            remainingTime -= current_timestep;
        }

        // TODO queue networking work

        // And sleep
        const auto sleepTime = current_timestep - remainingTime;
        if (sleepTime.count() > 0.0f) {
            std::this_thread::sleep_for(sleepTime);
        }
    }
    
    return 0;
}

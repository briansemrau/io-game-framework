#ifndef NETWORK_COMMON_H
#define NETWORK_COMMON_H

#include <random>
#include <string>


static constexpr auto TEST_DATACHANNEL = "test";
static constexpr auto DEFAULT_DATACHANNEL = "default";

std::string generateRandomIDStr(size_t length) {
    static thread_local std::mt19937 rng(static_cast<unsigned int>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
    static const std::string characters("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
    std::string id(length, '0');
    std::uniform_int_distribution<int> uniform(0, int(characters.size() - 1));
    std::generate(id.begin(), id.end(), [&]() { return characters.at(uniform(rng)); });
    return id;
}

using PeerID = uint64_t;

PeerID generateRandomPeerID() {
    static thread_local std::mt19937 rng(static_cast<unsigned int>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
    return std::uniform_int<uint64_t>()(rng);
}

#endif  // NETWORK_COMMON_H
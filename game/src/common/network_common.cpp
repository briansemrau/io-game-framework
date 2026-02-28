#include "network_common.h"

#include <chrono>
#include <random>
#include <string>

std::string generateRandomIDStr(size_t length) {
    static thread_local std::mt19937 rng(static_cast<unsigned int>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
    static const std::string characters("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
    std::string id(length, '0');
    std::uniform_int_distribution<int> uniform(0, int(characters.size() - 1));
    std::generate(id.begin(), id.end(), [&]() { return characters.at(uniform(rng)); });
    return id;
};

PeerID generateRandomPeerID() {
    static thread_local std::mt19937 rng(static_cast<unsigned int>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
    return std::uniform_int_distribution<uint64_t>()(rng);
}

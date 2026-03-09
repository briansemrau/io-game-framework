#ifndef NETWORK_COMMON_H
#define NETWORK_COMMON_H

#include <cstddef>
#include <memory>
#include <rtc/rtc.hpp>
#include <string>
#include <vector>

#include "types.h"

std::vector<std::string> getDefaultIceServerUrls();

static constexpr auto TEST_DATACHANNEL = "test";
static constexpr auto STATE_DATACHANNEL = "state";

struct NetworkConnection {
    std::shared_ptr<rtc::PeerConnection> peerConnection;
    std::shared_ptr<rtc::DataChannel> testDataChannel;
    std::shared_ptr<rtc::DataChannel> stateDataChannel;
};

extern PeerID generateRandomPeerID();

// struct EncodedEntityState {
//     EntityID id;
//     EntityType type;
//     std::vector<std::byte> data;
// };

// struct EncodedStateMessage {
//     uint64_t tick;
//     std::vector<EncodedEntityState> entityStates;
// };

#endif  // NETWORK_COMMON_H
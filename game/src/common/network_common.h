#ifndef NETWORK_COMMON_H
#define NETWORK_COMMON_H

#include <cstdint>
#include <memory>
#include <rtc/rtc.hpp>
#include <string>
#include <vector>

static constexpr auto TEST_DATACHANNEL = "test";
static constexpr auto STATE_DATACHANNEL = "state";

struct NetworkConnection {
    std::shared_ptr<rtc::PeerConnection> peerConnection;
    std::shared_ptr<rtc::DataChannel> testDataChannel;
    std::shared_ptr<rtc::DataChannel> stateDataChannel;
};

extern std::string generateRandomIDStr(size_t length);

using PeerID = uint64_t;

extern PeerID generateRandomPeerID();

std::vector<std::string> getDefaultIceServerUrls();

#endif  // NETWORK_COMMON_H
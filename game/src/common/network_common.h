#ifndef NETWORK_COMMON_H
#define NETWORK_COMMON_H

#include <string>

static constexpr auto TEST_DATACHANNEL = "test";
static constexpr auto STATE_DATACHANNEL = "state";

extern std::string generateRandomIDStr(size_t length);

using PeerID = uint64_t;

extern PeerID generateRandomPeerID();

#endif  // NETWORK_COMMON_H
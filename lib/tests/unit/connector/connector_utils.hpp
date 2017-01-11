#pragma once

#include <cpp-pcp-client/util/chrono.hpp>
#include <cpp-pcp-client/util/thread.hpp>
#include <leatherman/util/timer.hpp>

namespace PCPClient {

static constexpr int WS_TIMEOUT_MS { 5000 };
static constexpr uint32_t ASSOCIATION_TIMEOUT_S { 15 };
static constexpr uint32_t ASSOCIATION_REQUEST_TTL_S { 10 };
static constexpr uint32_t PONG_TIMEOUTS_BEFORE_RETRY { 3 };
static constexpr uint32_t PONG_TIMEOUT { 900 };

inline void wait_for(std::function<bool()> func, uint16_t pause_s = 2)
{
    leatherman::util::Timer timer {};

    while (!func() && timer.elapsed_seconds() < pause_s)
        Util::this_thread::sleep_for(Util::chrono::milliseconds(1));
}

}  // namespace PCPClient

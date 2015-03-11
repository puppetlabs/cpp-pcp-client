#ifndef CTHUN_CLIENT_SRC_CONNECTOR_TIMER_H_
#define CTHUN_CLIENT_SRC_CONNECTOR_TIMER_H_

#include <chrono>

// TODO(ale): move this to leatherman

namespace CthunClient {

/// A simple stopwatch/timer we can use for user feedback.
/// We use the std::chrono::steady_clock as we don't want to be
/// affected if the system clock changed around us (think ntp
/// skew/leapseconds).

class Timer {
  public:
    Timer() {
        reset();
    }

    /// @return Returns the time that has passed since last reset in seconds
    double elapsedSeconds() {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration<double>(now - start_).count();
    }

    /// Reset the clock
    void reset() {
        start_ = std::chrono::steady_clock::now();
    }

  private:
    std::chrono::time_point<std::chrono::steady_clock> start_;
};

}  // namespace CthunClient

#endif  // CTHUN_CLIENT_SRC_CONNECTOR_TIMER_H_

#ifndef CPP_PCP_CLIENT_SRC_CONNECTOR_TIMINGS_H_
#define CPP_PCP_CLIENT_SRC_CONNECTOR_TIMINGS_H_

#include <cpp-pcp-client/export.h>

#include <boost/chrono/chrono.hpp>

#include <string>

namespace PCPClient {

//
// ConnectionTimings
//

struct LIBCPP_PCP_CLIENT_EXPORT ConnectionTimings {
    using Duration_us = boost::chrono::duration<int, boost::micro>;

    boost::chrono::high_resolution_clock::time_point start;
    boost::chrono::high_resolution_clock::time_point tcp_pre_init;
    boost::chrono::high_resolution_clock::time_point tcp_post_init;
    boost::chrono::high_resolution_clock::time_point open;
    boost::chrono::high_resolution_clock::time_point close;

    bool connection_started { false };
    bool connection_failed { false };

    /// Sets the `start` time_point member to the current instant,
    /// the other time_points to epoch, and the flags to false
    void reset();

    /// Time interval to establish the TCP connection [us]
    Duration_us getTCPInterval() const;

    /// Time interval to perform the WebSocket handshake [us]
    Duration_us getHandshakeInterval() const;

    /// Time interval to establish the WebSocket connection [us]
    Duration_us getWebSocketInterval() const;

    /// Time interval until close or fail event [us]
    Duration_us getCloseInterval() const;

    /// Returns a string with the timings
    std::string toString() const;
};

}  // namespace PCPClient

#endif  // CPP_PCP_CLIENT_SRC_CONNECTOR_TIMINGS_H_

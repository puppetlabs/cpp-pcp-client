#ifndef CPP_PCP_CLIENT_SRC_CONNECTOR_TIMINGS_H_
#define CPP_PCP_CLIENT_SRC_CONNECTOR_TIMINGS_H_

#include <boost/chrono/chrono.hpp>

#include <string>

namespace PCPClient {

//
// ConnectionTimings
//

class ConnectionTimings {
  public:
    using Duration_us = boost::chrono::duration<int, boost::micro>;

    boost::chrono::high_resolution_clock::time_point start;
    boost::chrono::high_resolution_clock::time_point tcp_pre_init;
    boost::chrono::high_resolution_clock::time_point tcp_post_init;
    boost::chrono::high_resolution_clock::time_point open;
    boost::chrono::high_resolution_clock::time_point close;

    bool connection_started { false };
    bool connection_failed { false };

    ConnectionTimings();

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

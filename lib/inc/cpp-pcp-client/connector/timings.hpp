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
    boost::chrono::high_resolution_clock::time_point closing_handshake;
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

//
// AssociationTimings
//

struct LIBCPP_PCP_CLIENT_EXPORT AssociationTimings {
    using Duration_ms  = boost::chrono::duration<int, boost::milli>;
    using Duration_min = boost::chrono::minutes;

    boost::chrono::high_resolution_clock::time_point start;
    boost::chrono::high_resolution_clock::time_point association;
    boost::chrono::high_resolution_clock::time_point close;

    bool completed { false };
    bool success { false };
    bool closed { false };

    /// Sets the `start` time_point member to the current instant,
    /// the other time_points to epoch, and the flags to false
    void reset();

    /// Sets the Association timestamp member to the current instant
    /// if `closed` was not flagged, otherwise use the close one, then
    /// flags `completed` and sets `success` as specified
    void setCompleted(bool _success = true);

    /// Sets the session closure timestamp member to the current instant
    /// and flags `closed`
    void setClosed();

    /// Time interval to establish the TCP connection [ms]
    Duration_ms getAssociationInterval() const;

    /// Duration of the PCP Session [minutes]; it will return:
    ///  - a null duration, in case the Association was not completed;
    ///  - the (close - start) duration, in case the session was closed;
    ///  - the (now - start) duration, otherwise.
    Duration_min getOverallSessionInterval() const;

    /// Returns a string with the Association interval [ms];
    /// if `include_completion` is flagged and the Association was
    /// previously set as successfully completed, the string will
    /// also include the duration of the PCP Session
    std::string toString(bool include_completion = true) const;
};

}  // namespace PCPClient

#endif  // CPP_PCP_CLIENT_SRC_CONNECTOR_TIMINGS_H_

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
    using Duration_us  = boost::chrono::duration<int, boost::micro>;
    using Duration_min = boost::chrono::minutes;

    boost::chrono::high_resolution_clock::time_point start;
    boost::chrono::high_resolution_clock::time_point tcp_pre_init;
    boost::chrono::high_resolution_clock::time_point tcp_post_init;
    boost::chrono::high_resolution_clock::time_point open;
    boost::chrono::high_resolution_clock::time_point closing_handshake;
    boost::chrono::high_resolution_clock::time_point close;

    bool isOpen() const;
    bool isClosingStarted() const;
    bool isFailed() const;
    bool isClosed() const;

    /// Sets the `start` time_point member to the current instant,
    /// the other time_points to epoch, and all state flags to false
    void reset();

    /// Sets the `open` time_point member to the current instant
    /// and sets the related flag
    void setOpen();

    /// Sets the `closing_handshake` time_point member to the
    /// current instant and flags `closing_started`
    void setClosing();

    /// Sets the `close` time_point member to the current instant,
    /// flags `closed` and sets `connection_failed` to onFail_event
    void setClosed(bool onFail_event = false);

    /// Time interval to establish the TCP connection [us]
    Duration_us getTCPInterval() const;

    /// Time interval to perform the WebSocket Opening Handshake [us];
    /// it will return:
    ///  - a null duration, if the WebSocket is not open;
    ///  - the (tcp_post_init - tcp_pre_init) duration, otherwise.
    Duration_us getOpeningHandshakeInterval() const;

    /// Time interval to establish the overall WebSocket connection [us];
    /// it will return:
    ///  - a null duration, if the WebSocket is not open;
    ///  - the (open - start) duration, otherwise.
    Duration_us getWebSocketInterval() const;

    /// Time interval to perform the WebSocket Closing Handshake [us];
    /// it will return:
    ///  - a null duration, if:
    ///        * the Websocket is not open;
    ///        * the Closing Handshake was not started by this client;
    ///        * the Closing Handshake did not complete.
    ///  - the (close - closing_handshake) duration, otherwise.
    Duration_us getClosingHandshakeInterval() const;

    /// Duration of the WebSocket connection [minutes]; it will return:
    ///  - a null duration, if the WebSocket connection was not established;
    ///  - the (close - start) duration, if the connection was established
    ///    and, then, closed;
    ///  - the (now - start) duration, otherwise.
    Duration_min getOverallConnectionInterval_min() const;

    /// As getOverallConnectionInterval_min, but the duration is in us.
    Duration_us getOverallConnectionInterval_us() const;

    /// Returns a string with the WebSocket Opening Handshake timings
    std::string toString() const;

  private:
    bool _open            { false };
    bool _closing_started { false };
    bool _failed          { false };
    bool _closed          { false };

    std::string getOverallDurationTxt() const;
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
    /// the other time_points to epoch, and all flags to false
    void reset();

    /// Sets the Association time_point member to the current instant
    /// if `closed` was not flagged, otherwise use the close one, then
    /// flags `completed` and sets `success` as specified
    void setCompleted(bool _success = true);

    /// Sets the session closure time_point member to the current instant
    /// and flags `closed`
    void setClosed();

    /// Time interval to perform the Session Association [ms];
    /// it will return:
    ///  - a null duration, if the Session Association was not completed;
    ///  - the (association - start) duration, otherwise.
    Duration_ms getAssociationInterval() const;

    /// Duration of the PCP Session [minutes]; it will return:
    ///  - a null duration, in case the Association was not completed;
    ///  - the (close - start) duration, in case the session was closed;
    ///  - the (now - start) duration, otherwise.
    Duration_min getOverallSessionInterval_min() const;

    /// As getOverallSessionInterval_min, but the duration is in ms.
    Duration_ms getOverallSessionInterval_ms() const;

    /// Returns a string with the Association interval [ms];
    /// if `include_completion` is flagged and the Association was
    /// previously set as successfully completed, the string will
    /// also include the duration of the PCP Session
    std::string toString(bool include_completion = true) const;
};

}  // namespace PCPClient

#endif  // CPP_PCP_CLIENT_SRC_CONNECTOR_TIMINGS_H_

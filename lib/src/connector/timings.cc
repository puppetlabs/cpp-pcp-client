#include <cpp-pcp-client/connector/timings.hpp>

#include <leatherman/locale/locale.hpp>

namespace PCPClient {

namespace lth_loc = leatherman::locale;

//
// ConnectionTimings
//

void ConnectionTimings::reset()
{
    start         = boost::chrono::high_resolution_clock::now();
    tcp_pre_init  = boost::chrono::high_resolution_clock::time_point();
    tcp_post_init = boost::chrono::high_resolution_clock::time_point();
    open          = boost::chrono::high_resolution_clock::time_point();
    close         = boost::chrono::high_resolution_clock::time_point();
    connection_started = false;
    connection_failed  = false;
}

ConnectionTimings::Duration_us ConnectionTimings::getTCPInterval() const {
    return boost::chrono::duration_cast<ConnectionTimings::Duration_us>(
        tcp_pre_init - start);
}

ConnectionTimings::Duration_us ConnectionTimings::getHandshakeInterval() const {
    return boost::chrono::duration_cast<ConnectionTimings::Duration_us>(
        tcp_post_init - tcp_pre_init);
}

ConnectionTimings::Duration_us ConnectionTimings::getWebSocketInterval() const {
    return boost::chrono::duration_cast<ConnectionTimings::Duration_us>(
        open - start);
}

ConnectionTimings::Duration_us ConnectionTimings::getCloseInterval() const {
    return boost::chrono::duration_cast<ConnectionTimings::Duration_us>(
        close - start);
}

std::string ConnectionTimings::toString() const {
    if (connection_started)
        return lth_loc::format(
            "connection timings: TCP {1} us, WS handshake {2} us, overall {3} us",
            getTCPInterval().count(),
            getHandshakeInterval().count(),
            getWebSocketInterval().count());

    if (connection_failed)
        return lth_loc::format("time to failure {1} us", getCloseInterval().count());

    return lth_loc::translate("the endpoint has not been connected yet");
}

}  // namespace PCPClient

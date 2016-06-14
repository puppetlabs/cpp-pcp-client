#include <cpp-pcp-client/connector/timings.hpp>

#include <leatherman/locale/locale.hpp>

#include <stdint.h>

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

ConnectionTimings::Duration_us ConnectionTimings::getTCPInterval() const
{
    return boost::chrono::duration_cast<ConnectionTimings::Duration_us>(
        tcp_pre_init - start);
}

ConnectionTimings::Duration_us ConnectionTimings::getHandshakeInterval() const
{
    return boost::chrono::duration_cast<ConnectionTimings::Duration_us>(
        tcp_post_init - tcp_pre_init);
}

ConnectionTimings::Duration_us ConnectionTimings::getWebSocketInterval() const
{
    return boost::chrono::duration_cast<ConnectionTimings::Duration_us>(
        open - start);
}

ConnectionTimings::Duration_us ConnectionTimings::getCloseInterval() const
{
    return boost::chrono::duration_cast<ConnectionTimings::Duration_us>(
        close - start);
}

std::string ConnectionTimings::toString() const
{
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

//
// AssociationTimings
//

void AssociationTimings::reset()
{
    start       = boost::chrono::high_resolution_clock::now();
    association = boost::chrono::high_resolution_clock::time_point();
    close       = boost::chrono::high_resolution_clock::time_point();
    completed   = false;
    success     = false;
    closed      = false;
}

void AssociationTimings::setCompleted(bool _success)
{
    if (closed) {
        // The WebSocket connection was previously closed; use its timestamp
        association = close;
    } else {
        association = boost::chrono::high_resolution_clock::now();
    }

    completed = true;
    success   = _success;
}

void AssociationTimings::setClosed()
{
    close = boost::chrono::high_resolution_clock::now();
    closed = true;
}

AssociationTimings::Duration_ms AssociationTimings::getAssociationInterval() const
{
    return boost::chrono::duration_cast<AssociationTimings::Duration_ms>(
        association - start);
}

AssociationTimings::Duration_min AssociationTimings::getOverallSessionInterval() const
{
    if (!completed)
        return Duration_min::zero();

    if (closed)
        return boost::chrono::duration_cast<AssociationTimings::Duration_min>(
                close - association);

    return boost::chrono::duration_cast<AssociationTimings::Duration_min>(
            boost::chrono::high_resolution_clock::now() - association);
}

static std::string normalizeTimeInterval(uint32_t duration_min)
{
    auto hours   = duration_min / 60;
    auto minutes = duration_min % 60;

    if (hours > 0)
        return lth_loc::format("{1} hrs {2} min", hours, minutes);

    return lth_loc::format("{1} min", minutes);
}

std::string AssociationTimings::toString(bool include_completion) const
{
    if (!completed)
        return lth_loc::translate("the endpoint has not been associated yet");

    if (success) {
        if (closed) {
            return lth_loc::format(
                "PCP Session Association successfully completed in {1} ms, "
                "then closed after {2}",
                getAssociationInterval().count(),
                normalizeTimeInterval(getOverallSessionInterval().count()));
        } else if (include_completion) {
            return lth_loc::format(
                "PCP Session Association successfully completed in {1} ms; "
                "the current session has been associated for {2}",
                getAssociationInterval().count(),
                normalizeTimeInterval(getOverallSessionInterval().count()));
        } else {
            return lth_loc::format(
                "PCP Session Association successfully completed in {1} ms",
                getAssociationInterval().count());
        }
    } else {
        return lth_loc::format("PCP Session Association failed after {1} ms",
                               getAssociationInterval().count());
    }
}

}  // namespace PCPClient

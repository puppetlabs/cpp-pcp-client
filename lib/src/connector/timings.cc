#include <cpp-pcp-client/connector/timings.hpp>

#include <leatherman/locale/locale.hpp>

#include <stdint.h>

namespace PCPClient {

namespace lth_loc = leatherman::locale;

// Helper

static std::string normalizeTimeInterval(uint32_t duration_min)
{
    auto hours   = duration_min / 60;
    auto minutes = duration_min % 60;

    if (hours > 0)
        return lth_loc::format("{1} hrs {2} min", hours, minutes);

    return lth_loc::format("{1} min", minutes);
}

//
// ConnectionTimings
//

bool ConnectionTimings::isOpen() const { return _open; }
bool ConnectionTimings::isClosingStarted() const { return _closing_started; }
bool ConnectionTimings::isFailed() const { return _failed; }
bool ConnectionTimings::isClosed() const { return _closed; }

void ConnectionTimings::reset()
{
    start              = boost::chrono::high_resolution_clock::now();
    tcp_pre_init       = boost::chrono::high_resolution_clock::time_point();
    tcp_post_init      = boost::chrono::high_resolution_clock::time_point();
    open               = boost::chrono::high_resolution_clock::time_point();
    closing_handshake  = boost::chrono::high_resolution_clock::time_point();
    close              = boost::chrono::high_resolution_clock::time_point();

    _open            = false;
    _closing_started = false;
    _failed          = false;
    _closed          = false;
}

void ConnectionTimings::setOpen()
{
    open = boost::chrono::high_resolution_clock::now();
    _open = true;
}

void ConnectionTimings::setClosing()
{
    closing_handshake = boost::chrono::high_resolution_clock::now();
    _closing_started = true;
}

void ConnectionTimings::setClosed(bool onFail_event)
{
    close = boost::chrono::high_resolution_clock::now();
    _closed = true;
    _failed = onFail_event;
}

ConnectionTimings::Duration_us ConnectionTimings::getTCPInterval() const
{
    return boost::chrono::duration_cast<ConnectionTimings::Duration_us>(
        tcp_pre_init - start);
}

ConnectionTimings::Duration_us
ConnectionTimings::getOpeningHandshakeInterval() const
{
    if (!_open)
        return Duration_us::zero();

    return boost::chrono::duration_cast<ConnectionTimings::Duration_us>(
        tcp_post_init - tcp_pre_init);
}

ConnectionTimings::Duration_us ConnectionTimings::getWebSocketInterval() const
{
    if (!_open)
        return Duration_us::zero();

    return boost::chrono::duration_cast<ConnectionTimings::Duration_us>(
        open - start);
}

ConnectionTimings::Duration_us
ConnectionTimings::getClosingHandshakeInterval() const
{
    if (!_open || !_closing_started || !_closed)
        return Duration_us::zero();

    return boost::chrono::duration_cast<ConnectionTimings::Duration_us>(
            close - closing_handshake);
}

ConnectionTimings::Duration_min
ConnectionTimings::getOverallConnectionInterval_min() const
{
    if (!_open)
        return Duration_min::zero();

    if (!_closed)
        return boost::chrono::duration_cast<Duration_min>(
                boost::chrono::high_resolution_clock::now() - start);

    return boost::chrono::duration_cast<Duration_min>(close - start);
}

ConnectionTimings::Duration_us
ConnectionTimings::getOverallConnectionInterval_us() const
{
    if (!_open)
        return Duration_us::zero();

    if (_closed)
        return boost::chrono::duration_cast<Duration_us>(close - start);

    return boost::chrono::duration_cast<Duration_us>(
            boost::chrono::high_resolution_clock::now() - start);
}

std::string ConnectionTimings::toString() const
{
    if (_open)
        return lth_loc::format(
            "connection timings: TCP {1} us, WS handshake {2} us, overall {3} us",
            getTCPInterval().count(),
            getOpeningHandshakeInterval().count(),
            getWebSocketInterval().count());

    if (_failed)
        return lth_loc::format("time to failure {1}", getOverallDurationTxt());

    return lth_loc::translate("the endpoint has not been connected yet");
}

// Private helper
std::string ConnectionTimings::getOverallDurationTxt() const
{
    auto duration_min = getOverallConnectionInterval_min().count();

    if (duration_min)
        return normalizeTimeInterval(duration_min);

    return lth_loc::format("{1} us", getOverallConnectionInterval_us().count());
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

## 1.3.0

This is a feature and improvement release.

* [PCP-617](https://tickets.puppetlabs.com/browse/PCP-617) Fix compilation
with Boost 1.62.
* [PCP-582](https://tickets.puppetlabs.com/browse/PCP-582) Use
boost::exception_ptr instead of std::exception_ptr so we can build on ARM.
* [PCP-556](https://tickets.puppetlabs.com/browse/PCP-556) Implement PCP
access logging support.

Note: this release did not include changes from 1.2.1.

## 1.2.1

This is a maintenance release.

* [PCP-622](https://tickets.puppetlabs.com/browse/PCP-622) Fix crash on 32-bit
systems when receiving a packet claiming to have more data than can be stored
in 32-bit memory addressing.

## 1.2.0

This is a feature and improvement release.

* [PCP-543](https://tickets.puppetlabs.com/browse/PCP-543) Add CA mismatch as
a possible authentication failure cause when logging.
* [PCP-499](https://tickets.puppetlabs.com/browse/PCP-499) Ensure that the
WebSocket pong timeout is shorter than the connection check interval.
* [PCP-482](https://tickets.puppetlabs.com/browse/PCP-482) Update the tutorial
with a list of multiple brokers.
* [PCP-477](https://tickets.puppetlabs.com/browse/PCP-477) Synchronize onOpen
events between the onOpen handler and the synchronous blocking connectAndWait
function; this increases the function's responsiveness.
* [PCP-406](https://tickets.puppetlabs.com/browse/PCP-406) Update docs for
specifying the list of failover brokers.
* [PCP-466](https://tickets.puppetlabs.com/browse/PCP-466) Make Connector's
connection_ptr a protected member.
* [PCP-440](https://tickets.puppetlabs.com/browse/PCP-440) Retrieve WebSocket
closing handshake timing stats.
* [PCP-411](https://tickets.puppetlabs.com/browse/PCP-411) Retrieve and expose
PCP Session Association timing stats.
* [PCP-410](https://tickets.puppetlabs.com/browse/PCP-410) Expose the WebSocket
opening handshake timing stats.
* [PCP-463](https://tickets.puppetlabs.com/browse/PCP-463) Add the blocking
`monitorConnection` function; propagate exceptions thrown in the monitor thread.
* [PCP-455](https://tickets.puppetlabs.com/browse/PCP-455) Add unit tests for
the Connector class and new functionalities to MockServer.
* [PCP-452](https://tickets.puppetlabs.com/browse/PCP-452) Allow configuring a
timeout for the entire Session Association process, in addition to the
`association_request_timeout_s`.
* [PCP-454](https://tickets.puppetlabs.com/browse/PCP-454) Add the non blocking
Connector::startMonitoring function.
* [PCP-423](https://tickets.puppetlabs.com/browse/PCP-423) Detect and close
unresponsive connections.
* [PCP-417](https://tickets.puppetlabs.com/browse/PCP-417) Add the
`association_timeout_s` for specifying the PCP Association Request TTL and
rename `connection_timeout` to `ws_connection_timeout_ms`.
* [PCP-425](https://tickets.puppetlabs.com/browse/PCP-425) The Connection::close
function now pauses before closing the underlying connection if it's in a
transient state (opening or closing).
* [PCP-424](https://tickets.puppetlabs.com/browse/PCP-424) Don't let
Connection::connect return before the configured connection_timeout.
* [PCP-412](https://tickets.puppetlabs.com/browse/PCP-412) Log when switching to
failover broker.
* [#161](https://github.com/puppetlabs/cpp-pcp-client/pull/161) HA support PR.
Also enables both shared and static library builds in CI.
* [PCP-383](https://tickets.puppetlabs.com/browse/PCP-383) Implement broker
failover.

## 1.1.4

This is a maintenance release.

* [PCP-400](https://tickets.puppetlabs.com/browse/PCP-400) Log a warning when
SSL verification fails.
* [#190](https://github.com/puppetlabs/cpp-pcp-client/pull/190) Bump to
Leatherman 0.7.4.
* [#173](https://github.com/puppetlabs/cpp-pcp-client/pull/173) Bump to
Leatherman 0.7.0.

## 1.1.3

This is a feature and maintenance release.

* [#164](https://github.com/puppetlabs/cpp-pcp-client/pull/164) Fix AppVeyor
builds.
* [PCP-371](https://tickets.puppetlabs.com/browse/PCP-371) Enable i18n support.
* [PCP-270](https://tickets.puppetlabs.com/browse/PCP-270) Remove outsourced
pthread dependency.
* [#155](https://github.com/puppetlabs/cpp-pcp-client/pull/155) Add CMake
building option to toggle static/shared libraries.
* [#153](https://github.com/puppetlabs/cpp-pcp-client/pull/153) Add Maintenance
section to README.
* [PCP-372](https://tickets.puppetlabs.com/browse/PCP-372) Support building with
Leatherman DLLs (this prevents occasional scrambled log entries on Windows).

## 1.1.2

This is an improvement release.

* [PCP-374](https://tickets.puppetlabs.com/browse/PCP-374) Update Catch and
Leatherman libraries.
* [PCP-344](https://tickets.puppetlabs.com/browse/PCP-344) Add an extension
point for setting a `ttl_expired` callback; deal with
`ttl_expired` related to Session Association requests.
* [PCP-338](https://tickets.puppetlabs.com/browse/PCP-338) Improve Session
Association logic; handle Session Association failures.

## 1.1.1

This version integrates the changes made for 1.0.4.

## 1.1.0

This is an improvement release. It is still based on the
[PCP v1.0 protocol](https://github.com/puppetlabs/pcp-specifications/tree/master/pcp/versions/1.0).

* [PCP-244](https://tickets.puppetlabs.com/browse/PCP-244) Improve building
instructions on README and remove Makefile.
* [#133](https://github.com/puppetlabs/cpp-pcp-client/pull/133) Add dependency
on Boost.Chrono.
* [PCP-269](https://tickets.puppetlabs.com/browse/PCP-269) Add dependencies on
external projects on CMakeLists to guarantee a successful build.
* [PCP-209](https://tickets.puppetlabs.com/browse/PCP-209) Unvendor leatherman
and add dependency on the installed leatherman libraries.
* [PCP-217](https://tickets.puppetlabs.com/browse/PCP-217) The function for
sending messages now returns the message ID.
* [PCP-216](https://tickets.puppetlabs.com/browse/PCP-216) Improve the logic
  that establishes the WebSocket connection.
* [PCP-192](https://tickets.puppetlabs.com/browse/PCP-192) Add in-reply-to entry
  to PCP envelope schema definition.
* [PCP-177](https://tickets.puppetlabs.com/browse/PCP-177) Allow cpp-pcp-client
  user to configure the WebSocket connection timeout.
* [PCP-137](https://tickets.puppetlabs.com/browse/PCP-137) Default to setting
  CMAKE_BUILD_TYPE to Release.
* [#116](https://github.com/puppetlabs/cpp-pcp-client/pull/116) Log Broker URI
  on Websocket onOpen event.
* [PCP-121](https://tickets.puppetlabs.com/browse/PCP-121) Only export needed
  symbols.

## 1.0.4

This is a security release.

* [PCP-318](https://tickets.puppetlabs.com/browse/PCP-318) During the TLS
 initialization of the WebSocket connection, ensure that the broker certificate
 is signed by the configured CA and that the reported identity of the broker
 matches the common name of the configured WebSocket URI.

## 1.0.3

This was an cancelled release and the tag should not be used.

## 1.0.2

This was an cancelled release and the tag should not be used.

## 1.0.1

This is an improvement release.

* [PCP-5](https://tickets.puppetlabs.com/browse/PCP-5) Improve
  visibility and wording of log messages.

## 1.0.0

This is the first release.

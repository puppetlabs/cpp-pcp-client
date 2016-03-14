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

This is a security release

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

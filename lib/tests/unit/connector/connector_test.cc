#include "tests/test.hpp"
#include "tests/unit/connector/certs.hpp"
#include "tests/unit/connector/mock_server.hpp"

#include <cpp-pcp-client/connector/connector.hpp>
#include <cpp-pcp-client/connector/client_metadata.hpp>
#include <cpp-pcp-client/connector/errors.hpp>

#include <cpp-pcp-client/util/thread.hpp>
#include <cpp-pcp-client/util/chrono.hpp>

#include <leatherman/util/timer.hpp>

#include <memory>
#include <atomic>
#include <functional>

namespace PCPClient {

namespace lth_util = leatherman::util;

static constexpr int WS_TIMEOUT_MS { 5000 };
static constexpr uint32_t ASSOCIATION_TIMEOUT_S { 15 };
static constexpr uint32_t ASSOCIATION_REQUEST_TTL_S { 10 };
static constexpr uint32_t PONG_TIMEOUTS_BEFORE_RETRY { 3 };
static constexpr uint32_t PONG_TIMEOUT { 30000 };

TEST_CASE("Connector::Connector", "[connector]") {
    SECTION("can instantiate") {
        REQUIRE_NOTHROW(Connector("wss://localhost:8142/pcp", "test_client",
                                  getCaPath(), getCertPath(), getKeyPath(),
                                  WS_TIMEOUT_MS,
                                  ASSOCIATION_TIMEOUT_S, ASSOCIATION_REQUEST_TTL_S,
                                  PONG_TIMEOUTS_BEFORE_RETRY, PONG_TIMEOUT));
    }
}

static void wait_for(std::function<bool()> func, uint16_t pause_s = 2)
{
    lth_util::Timer timer {};

    while (!func() && timer.elapsed_seconds() < pause_s)
        Util::this_thread::sleep_for(Util::chrono::milliseconds(1));
}

TEST_CASE("Connector::getConnectionTimings", "[connector]") {
    Connector c { "wss://localhost:8142/pcp",
                  "test_client",
                  getCaPath(), getCertPath(), getKeyPath(),
                  WS_TIMEOUT_MS, ASSOCIATION_TIMEOUT_S,
                  ASSOCIATION_REQUEST_TTL_S,
                  PONG_TIMEOUTS_BEFORE_RETRY, PONG_TIMEOUT };

    SECTION("can get WebSocket timings from the Connection instance") {
        REQUIRE_NOTHROW(c.getConnectionTimings());
    }

    SECTION("timings will say if the connection was not established") {
        auto timings = c.getConnectionTimings();

        REQUIRE_FALSE(timings.connection_started);
        REQUIRE_FALSE(timings.connection_failed);
    }
}

TEST_CASE("Connector::connect", "[connector]") {
    MockServer mock_server;
    bool connected = false;
    mock_server.set_open_handler(
        [&connected](websocketpp::connection_hdl hdl) {
            connected = true;
        });
    mock_server.go();
    auto port = mock_server.port();

    SECTION("successfully connects") {
        Connector c { "wss://localhost:" + std::to_string(port) + "/pcp",
                      "test_client",
                      getCaPath(), getCertPath(), getKeyPath(),
                      WS_TIMEOUT_MS, ASSOCIATION_TIMEOUT_S,
                      ASSOCIATION_REQUEST_TTL_S,
                      PONG_TIMEOUTS_BEFORE_RETRY, PONG_TIMEOUT };
        REQUIRE_FALSE(connected);
        REQUIRE_NOTHROW(c.connect(1));

        // NB: Connector::connect is blocking
        REQUIRE(c.isConnected());
        REQUIRE(connected);

        wait_for([&c](){return c.isAssociated();});
        auto ws_i = c.getConnectionTimings().getWebSocketInterval();

        REQUIRE(c.isAssociated());

        REQUIRE(ConnectionTimings::Duration_us::zero() < ws_i);
    }
}

TEST_CASE("Connector Monitoring Task", "[connector]") {
    bool use_blocking_call;

    SECTION("reconnects after the broker stops") {
        SECTION("when using NON blocking calls (start/stopMonitoring") {
            use_blocking_call = false;
        }

        SECTION("when using blocking calls (monitorConnection and dtor)") {
            use_blocking_call = true;
        }

        std::unique_ptr<Connector> c_ptr;
        uint16_t port;
        Util::thread monitor;

        {
            MockServer mock_server;
            bool connected = false;
            mock_server.set_open_handler(
                    [&connected](websocketpp::connection_hdl hdl) {
                        connected = true;
                    });
            mock_server.go();
            port = mock_server.port();

            c_ptr.reset(new Connector("wss://localhost:" + std::to_string(port) + "/pcp",
                                      "test_client",
                                      getCaPath(), getCertPath(), getKeyPath(),
                                      WS_TIMEOUT_MS, ASSOCIATION_TIMEOUT_S,
                                      ASSOCIATION_REQUEST_TTL_S,
                                      PONG_TIMEOUTS_BEFORE_RETRY, PONG_TIMEOUT));

            REQUIRE_FALSE(connected);
            REQUIRE_NOTHROW(c_ptr->connect(1));
            REQUIRE(c_ptr->isConnected());
            REQUIRE(connected);

            // Monitor with infinite retries and 1 s between each WebSocket ping
            if (use_blocking_call) {
                monitor = Util::thread(
                    [&c_ptr]() {
                        REQUIRE_NOTHROW(c_ptr->monitorConnection(0, 1));
                    });
            } else {
                REQUIRE_NOTHROW(c_ptr->startMonitoring(0, 1));
            }
        }

        // The broker does not exist anymore (RAII)
        wait_for([&c_ptr](){return !c_ptr->isConnected();});

        REQUIRE_FALSE(c_ptr->isConnected());

        MockServer mock_server {port};
        mock_server.go();

        wait_for([&c_ptr](){return c_ptr->isConnected();});

        REQUIRE(c_ptr->isConnected());

        wait_for([&c_ptr](){return c_ptr->isAssociated();});

        REQUIRE(c_ptr->isAssociated());

        REQUIRE_NOTHROW(c_ptr->stopMonitoring());

        if (use_blocking_call) {
            if (monitor.joinable()) {
                // Ensure monitoring halts completely before we attempt to
                // destroy the connection. Otherwise destruction will attempt to
                // cleanup the object before startMonitorTask has exited.
                monitor.join();
            } else {
                FAIL("the monitor thread is not joinable");
            }
        }
    }

    SECTION("reconnects after 1 pong timeout") {
        SECTION("when using NON blocking calls (start/stopMonitoring") {
            use_blocking_call = false;
        }

        SECTION("when using blocking calls (monitorConnection and dtor)") {
            use_blocking_call = true;
        }

        MockServer mock_server;
        Util::thread monitor;
        std::atomic<int> num_connections {0};
        std::atomic<int> num_pings {0};
        mock_server.set_open_handler(
                [&num_connections](websocketpp::connection_hdl hdl) {
                    ++num_connections;
                });
        mock_server.set_ping_handler(
                [&num_pings](websocketpp::connection_hdl hdl, std::string) -> bool {
                    ++num_pings;
                    return false;
                });
        mock_server.go();
        auto port = mock_server.port();

        // Setting the pong timeout to 500 ms and the Association
        // timeout to 1 s (Connector::connect will hang waiting for
        // for the Association completion that won't happen because
        // we'll be done as soon as num_connections > 1)
        Connector c { "wss://localhost:" + std::to_string(port) + "/pcp",
                      "test_client",
                      getCaPath(), getCertPath(), getKeyPath(),
                      WS_TIMEOUT_MS, 1, ASSOCIATION_REQUEST_TTL_S, 1, 500 };

        REQUIRE(num_connections == 0);
        REQUIRE(num_pings == 0);
        REQUIRE_NOTHROW(c.connect(1));
        REQUIRE(c.isConnected());
        REQUIRE(num_connections == 1);

        // Monitor with infinite retries and 1 s between each WebSocket ping
        if (use_blocking_call) {
            monitor = Util::thread(
                [&c]() {
                    REQUIRE_NOTHROW(c.monitorConnection(0, 1));
                });
        } else {
            REQUIRE_NOTHROW(c.startMonitoring(0, 1));
        }

        // After 1 pong timeout, the Connector should close and reconnect
        wait_for([&num_connections](){return num_connections > 1;}, 30);

        REQUIRE(num_connections > 1);
        REQUIRE(num_pings > 0);

        REQUIRE_NOTHROW(c.stopMonitoring());

        if (use_blocking_call) {
            if (monitor.joinable()) {
                // Ensure monitoring halts completely before we attempt to
                // destroy the connection. Otherwise destruction will attempt to
                // cleanup the object before startMonitorTask has exited.
                monitor.join();
            } else {
                FAIL("the monitor thread is not joinable");
            }
        }
    }

    SECTION("in case of an exception caught by the monitoring task") {
        SECTION("when using NON blocking calls, stopMonitoring rethrows the exception") {
            use_blocking_call = false;
        }

        SECTION("when using blocking calls, monitorConnection propagates the exception") {
            use_blocking_call = true;
        }

        std::unique_ptr<MockServer> mock_server_ptr {new MockServer()};
        Util::thread monitor;
        bool connected {false};
        mock_server_ptr->set_open_handler(
                [&connected](websocketpp::connection_hdl hdl) {
                    connected = true;
                });
        mock_server_ptr->go();
        auto port = mock_server_ptr->port();

        // Setting pong timeout to 500 ms and Association timeout to 1 s
        Connector c { "wss://localhost:" + std::to_string(port) + "/pcp",
                      "test_client",
                      getCaPath(), getCertPath(), getKeyPath(),
                      900, 1, ASSOCIATION_REQUEST_TTL_S, 1, PONG_TIMEOUT };

        REQUIRE_FALSE(connected);
        REQUIRE_NOTHROW(c.connect(1));
        REQUIRE(c.isConnected());

        // Monitor with 1 connection retry and 1 s between each WebSocket ping
        if (use_blocking_call) {
            monitor = Util::thread(
                    [&]() {
                        REQUIRE_THROWS_AS(c.monitorConnection(1, 1),
                                          connection_fatal_error);
                    });
        } else {
            REQUIRE_NOTHROW(c.startMonitoring(1, 1));
        }

        // Destroy the broker
        mock_server_ptr.reset(nullptr);

        // Wait for the connection to drop plus 3 seconds to let the monitor
        // task fail (monitoring period is 1 s)
        wait_for([&c](){return !c.isConnected();});
        Util::this_thread::sleep_for(Util::chrono::seconds(3));

        if (!use_blocking_call)
            REQUIRE_THROWS_AS(c.stopMonitoring(), connection_fatal_error);

        if (use_blocking_call && monitor.joinable()) {
            // Ensure monitoring halts completely before we attempt to
            // destroy the connection. Otherwise destruction will attempt to
            // cleanup the object before startMonitorTask has exited
            monitor.join();
        }
    }
}

}  // namespace PCPClient

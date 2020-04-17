#include "tests/test.hpp"
#include "tests/unit/connector/certs.hpp"
#include "tests/unit/connector/mock_server.hpp"
#include "tests/unit/connector/connector_utils.hpp"

#include <cpp-pcp-client/connector/connector_base.hpp>
#include <cpp-pcp-client/connector/errors.hpp>
#include <cpp-pcp-client/connector/timings.hpp>

#include <cpp-pcp-client/util/chrono.hpp>
#include <cpp-pcp-client/util/thread.hpp>

#include <memory>
#include <atomic>
#include <functional>

using namespace PCPClient;

// Concrete implementation of ContectorBase for testing that ignores messages.
class ConnectorTester : public ConnectorBase {
  public:
    using ConnectorBase::ConnectorBase;
  protected:
    void processMessage(const std::string& msg_txt) override {}
};

TEST_CASE("ConnectorBase::ConnectorBase", "[connector]") {
    SECTION("can instantiate (with crl, ws_proxy constructor)") {
        REQUIRE_NOTHROW(ConnectorTester(std::vector<std::string> {"wss://localhost:8142/pcp"},
            "test_client",
            getCaPath(), getCertPath(), getKeyPath(), getEmptyCrlPath(), "",
            WS_TIMEOUT_MS, PONG_TIMEOUTS_BEFORE_RETRY, PONG_TIMEOUT));
    }
}

TEST_CASE("ConnectorBase::getConnectionTimings", "[connector]") {
    ConnectorTester c { std::vector<std::string> { "wss://localhost:8142/pcp" },
        "test_client",
        getCaPath(), getCertPath(), getKeyPath(),
        WS_TIMEOUT_MS,
        PONG_TIMEOUTS_BEFORE_RETRY, PONG_TIMEOUT };

    SECTION("can get WebSocket timings from the Connection instance") {
        REQUIRE_NOTHROW(c.getConnectionTimings());
    }

    SECTION("timings will say if the connection was not established") {
        auto ws_timings = c.getConnectionTimings();

        REQUIRE_FALSE(ws_timings.isOpen());
        REQUIRE_FALSE(ws_timings.isClosingStarted());
        REQUIRE_FALSE(ws_timings.isFailed());
        REQUIRE_FALSE(ws_timings.isClosed());
    }
}

TEST_CASE("ConnectorBase::connect", "[connector]") {
    MockServer mock_server;
    bool connected = false;
    mock_server.set_open_handler(
        [&connected](websocketpp::connection_hdl hdl) {
            connected = true;
        });
    mock_server.go();
    auto port = mock_server.port();

    SECTION("successfully connects and update WebSocket timings") {
        ConnectorTester c { std::vector<std::string> { "wss://localhost:" + std::to_string(port) + "/pcp" },
            "test_client",
            getCaPath(), getCertPath(), getKeyPath(),
            WS_TIMEOUT_MS, PONG_TIMEOUTS_BEFORE_RETRY, PONG_TIMEOUT };
        REQUIRE_FALSE(connected);
        REQUIRE_NOTHROW(c.connect(1));

        // NB: ConnectorBase::connect is blocking, but the onOpen handler isn't
        wait_for([&](){return connected;});
        REQUIRE(c.isConnected());
        REQUIRE(connected);

        auto ws_timings  = c.getConnectionTimings();
        auto us_zero     = ConnectionTimings::Duration_us::zero();
        auto min_zero    = ConnectionTimings::Duration_min::zero();

        REQUIRE(us_zero   < ws_timings.getOpeningHandshakeInterval());
        REQUIRE(us_zero   < ws_timings.getWebSocketInterval());
        REQUIRE(us_zero   < ws_timings.getOverallConnectionInterval_us());
        REQUIRE(min_zero == ws_timings.getOverallConnectionInterval_min());
        REQUIRE(us_zero  == ws_timings.getClosingHandshakeInterval());
    }
}

TEST_CASE("ConnectorTester monitor connection throws if pong timeout would be ignored", "[connector]") {
    MockServer mock_server;
    mock_server.go();
    auto port = mock_server.port();

    ConnectorTester c { std::vector<std::string> { "wss://localhost:" + std::to_string(port) + "/pcp" },
        "test_client", getCaPath(), getCertPath(), getKeyPath(),
        WS_TIMEOUT_MS, PONG_TIMEOUTS_BEFORE_RETRY, 2000 };
    c.connect(1);

    SECTION("monitorConnection ping every second") {
        REQUIRE_THROWS_AS(c.monitorConnection(0, 1), connection_config_error);
    }

    SECTION("monitorConnection ping every 2 seconds") {
        REQUIRE_THROWS_AS(c.monitorConnection(0, 2), connection_config_error);
    }

    SECTION("startMonitoring ping every second") {
        REQUIRE_THROWS_AS(c.startMonitoring(0, 1), connection_config_error);
    }

    SECTION("startMonitoring ping every 2 seconds") {
        REQUIRE_THROWS_AS(c.startMonitoring(0, 2), connection_config_error);
    }
}

TEST_CASE("ConnectorTester Monitoring Task", "[connector]") {
    bool use_blocking_call;

    SECTION("reconnects after the broker stops (also updates Association metrics)") {
        SECTION("when using NON blocking calls (start/stopMonitoring)") {
            use_blocking_call = false;
        }

        SECTION("when using blocking calls (monitorConnection and dtor)") {
            use_blocking_call = true;
        }

        std::unique_ptr<ConnectorBase> c_ptr;
        uint16_t port;
        Util::thread monitor;

        {
            MockServer mock_server;
            mock_server.go();
            port = mock_server.port();

            c_ptr.reset(new ConnectorTester(std::vector<std::string> { "wss://localhost:" + std::to_string(port) + "/pcp" },
                "test_client",
                getCaPath(), getCertPath(), getKeyPath(),
                WS_TIMEOUT_MS, PONG_TIMEOUTS_BEFORE_RETRY, PONG_TIMEOUT));

            REQUIRE_NOTHROW(c_ptr->connect(1));
            REQUIRE(c_ptr->isConnected());

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
        auto ws_timings  = c_ptr->getConnectionTimings();
        auto us_zero     = ConnectionTimings::Duration_us::zero();

        REQUIRE_FALSE(c_ptr->isConnected());
        REQUIRE_FALSE(ws_timings.isFailed());
        REQUIRE(ws_timings.isClosed());
        // NB: We didn't initiate the WS Closing Handshake
        REQUIRE(us_zero == ws_timings.getClosingHandshakeInterval());

        MockServer mock_server {port};
        mock_server.go();

        wait_for([&c_ptr](){return c_ptr->isConnected();});

        REQUIRE(c_ptr->isConnected());

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
        SECTION("when using NON blocking calls (start/stopMonitoring)") {
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

        ConnectorTester c { std::vector<std::string> { "wss://localhost:" + std::to_string(port) + "/pcp" },
            "test_client",
            getCaPath(), getCertPath(), getKeyPath(),
            WS_TIMEOUT_MS, 1, 500 };

        REQUIRE(num_connections == 0);
        REQUIRE(num_pings == 0);
        REQUIRE_NOTHROW(c.connect(1));
        REQUIRE(c.isConnected());

        // Client registering the connection doesn't mean server open handler was called, wait for it.
        wait_for([&num_connections](){return num_connections == 1;}, 1);
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

        // After 1 pong timeout, the ConnectorBase should close and reconnect
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
        mock_server_ptr->go();
        auto port = mock_server_ptr->port();

        // Setting pong timeout to 900 ms and Association timeout to 1 s
        ConnectorTester c { std::vector<std::string> { "wss://localhost:" + std::to_string(port) + "/pcp" },
            "test_client",
            getCaPath(), getCertPath(), getKeyPath(),
            900, 1, PONG_TIMEOUT };

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

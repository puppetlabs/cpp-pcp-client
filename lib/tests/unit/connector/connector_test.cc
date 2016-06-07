#include "tests/test.hpp"
#include "tests/unit/connector/certs.hpp"
#include "tests/unit/connector/mock_server.hpp"

#include <cpp-pcp-client/connector/connector.hpp>
#include <cpp-pcp-client/connector/client_metadata.hpp>
#include <cpp-pcp-client/connector/errors.hpp>

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

TEST_CASE("Connector::Connector", "[connector]") {
    SECTION("can instantiate") {
        REQUIRE_NOTHROW(Connector("wss://localhost:8142/pcp", "test_client",
                                  getCaPath(), getCertPath(), getKeyPath(),
                                  WS_TIMEOUT_MS,
                                  ASSOCIATION_TIMEOUT_S, ASSOCIATION_REQUEST_TTL_S,
                                  PONG_TIMEOUTS_BEFORE_RETRY));
    }
}

static void wait_for(std::function<bool()> func, uint16_t pause_s = 2)
{
    lth_util::Timer timer {};

    while (!func() && timer.elapsed_seconds() < pause_s)
        Util::this_thread::sleep_for(Util::chrono::milliseconds(1));
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
                      ASSOCIATION_REQUEST_TTL_S, PONG_TIMEOUTS_BEFORE_RETRY };
        REQUIRE_FALSE(connected);
        REQUIRE_NOTHROW(c.connect(1));

        // NB: Connector::connect is blocking
        REQUIRE(c.isConnected());
        REQUIRE(connected);

        wait_for([&c](){return c.isAssociated();});

        REQUIRE(c.isAssociated());
    }
}

TEST_CASE("Connector::startMonitoring", "[connector]") {
    SECTION("reconnects after the broker stops") {
        std::unique_ptr<Connector> c_ptr;
        uint16_t port;

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
                                      PONG_TIMEOUTS_BEFORE_RETRY));

            REQUIRE_FALSE(connected);
            REQUIRE_NOTHROW(c_ptr->connect(1));
            REQUIRE(c_ptr->isConnected());
            REQUIRE(connected);

            // Infinite retries; 1 s between each WebSocket ping
            REQUIRE_NOTHROW(c_ptr->startMonitoring(0, 1));
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
    }

    SECTION("reconnects after 1 pong timeout") {
        MockServer mock_server;
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

        // Infinite retries; 1 s between each WebSocket ping
        REQUIRE_NOTHROW(c.startMonitoring(0, 1));

        // After 1 pong timeout, the Connector should close and reconnect
        wait_for([&num_connections](){return num_connections > 1;}, 30);

        REQUIRE(num_connections > 1);
        REQUIRE(num_pings > 0);
    }
}

}  // namespace PCPClient

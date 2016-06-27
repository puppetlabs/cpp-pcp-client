#include "tests/test.hpp"
#include "tests/unit/connector/certs.hpp"
#include "tests/unit/connector/mock_server.hpp"

#include <cpp-pcp-client/connector/connection.hpp>
#include <cpp-pcp-client/connector/client_metadata.hpp>
#include <cpp-pcp-client/connector/errors.hpp>
#include <cpp-pcp-client/connector/timings.hpp>

#include <cpp-pcp-client/util/chrono.hpp>

#include <leatherman/util/timer.hpp>

#include <memory>

namespace PCPClient {

namespace lth_util = leatherman::util;

static constexpr int WS_TIMEOUT { 5000 };
static constexpr uint32_t ASSOCIATION_TIMEOUT_S { 15 };
static constexpr uint32_t ASSOCIATION_REQUEST_TTL_S { 10 };
static constexpr uint32_t PONG_TIMEOUTS_BEFORE_RETRY { 3 };
static constexpr uint32_t PONG_TIMEOUT_MS { 30000 };

TEST_CASE("Connection::connect errors", "[connection]") {
    SECTION("throws a connection_processing_error if the broker url is "
            "not a valid WebSocket url") {
        ClientMetadata c_m { "test_client", getCaPath(), getCertPath(),
                             getKeyPath(), 6,
                             ASSOCIATION_TIMEOUT_S, ASSOCIATION_REQUEST_TTL_S,
                             PONG_TIMEOUTS_BEFORE_RETRY, PONG_TIMEOUT_MS };
        // NB: the dtor will wait for the 6 ms specified above
        Connection connection { "foo", c_m };

        REQUIRE_THROWS_AS(connection.connect(),
                          connection_processing_error);
    }
}

TEST_CASE("Connection timings", "[connection]") {
    ClientMetadata c_m { "test_client", getCaPath(), getCertPath(),
                         getKeyPath(), WS_TIMEOUT,
                         ASSOCIATION_TIMEOUT_S, ASSOCIATION_REQUEST_TTL_S,
                         PONG_TIMEOUTS_BEFORE_RETRY, PONG_TIMEOUT_MS };

    SECTION("can stringify timings") {
        Connection connection { "wss://localhost:8142/pcp", c_m };
        REQUIRE_NOTHROW(connection.timings.toString());
    }

    SECTION("WebSocket timings are retrieved") {
        auto start = boost::chrono::high_resolution_clock::now();
        MockServer mock_server;
        mock_server.go();
        Connection connection {
            "wss://localhost:" + std::to_string(mock_server.port()) + "/pcp", c_m };
        connection.connect(1);
        auto tot = boost::chrono::duration_cast<ConnectionTimings::Duration_us>(
                boost::chrono::high_resolution_clock::now() - start);
        auto duration_zero = ConnectionTimings::Duration_us::zero();

        REQUIRE(duration_zero < connection.timings.getTCPInterval());
        REQUIRE(duration_zero < connection.timings.getOpeningHandshakeInterval());
        REQUIRE(duration_zero < connection.timings.getWebSocketInterval());
        REQUIRE(duration_zero < connection.timings.getOverallConnectionInterval_us());
        REQUIRE(duration_zero == connection.timings.getClosingHandshakeInterval());
        REQUIRE(connection.timings.getWebSocketInterval() < tot);
    }
}

static void let_connection_stop(Connection const& connection, int timeout = 2)
{
    lth_util::Timer timer {};
    while (connection.getConnectionState() == ConnectionState::open
            && timer.elapsed_seconds() < timeout)
        Util::this_thread::sleep_for(Util::chrono::milliseconds(1));
    REQUIRE(connection.getConnectionState() != ConnectionState::open);
}

TEST_CASE("Connection::connect", "[connection]") {
    SECTION("successfully connects, closes, and sets Closing Handshake timings") {
        ClientMetadata c_m { "test_client", getCaPath(), getCertPath(),
                             getKeyPath(), WS_TIMEOUT,
                             ASSOCIATION_TIMEOUT_S, ASSOCIATION_REQUEST_TTL_S,
                             PONG_TIMEOUTS_BEFORE_RETRY, PONG_TIMEOUT_MS };

        MockServer mock_server;
        bool connected = false;
        mock_server.set_open_handler([&connected](websocketpp::connection_hdl hdl) {
            connected = true;
        });
        mock_server.go();

        Connection connection {
            "wss://localhost:" + std::to_string(mock_server.port()) + "/pcp", c_m };
        connection.connect(10);
        REQUIRE(connected);

        connection.close();
        auto duration_zero = ConnectionTimings::Duration_us::zero();

        // Let's wait for the connection to close and retrieve the duration
        // NB: we can't use let_connection_stop as the connection state
        // will become `closing`
        lth_util::Timer timer {};

        while (connection.getConnectionState() != ConnectionState::closed
               && timer.elapsed_seconds() < 2)
            Util::this_thread::sleep_for(Util::chrono::milliseconds(1));

        REQUIRE(connection.getConnectionState() == ConnectionState::closed);
        REQUIRE(duration_zero < connection.timings.getClosingHandshakeInterval());
    }

    SECTION("successfully connects to failover broker") {
        ClientMetadata c_m { "test_client", getCaPath(), getCertPath(),
                             getKeyPath(), WS_TIMEOUT,
                             ASSOCIATION_TIMEOUT_S, ASSOCIATION_REQUEST_TTL_S,
                             PONG_TIMEOUTS_BEFORE_RETRY, PONG_TIMEOUT_MS };

        MockServer mock_server;
        bool connected = false;
        mock_server.set_open_handler([&connected](websocketpp::connection_hdl hdl) {
            connected = true;
        });
        mock_server.go();

        auto port = mock_server.port();
        Connection connection {
            std::vector<std::string> { "wss://localhost:" + std::to_string(port+1) + "/pcp",
                                       "wss://localhost:" + std::to_string(port) + "/pcp" },
            c_m };
        connection.connect(30);
        REQUIRE(connected);

        connection.close();
        let_connection_stop(connection);
    }

    SECTION("successfully connects to failover when primary broker disappears") {
        ClientMetadata c_m { "test_client", getCaPath(), getCertPath(),
                             getKeyPath(), WS_TIMEOUT,
                             ASSOCIATION_TIMEOUT_S, ASSOCIATION_REQUEST_TTL_S,
                             PONG_TIMEOUTS_BEFORE_RETRY, PONG_TIMEOUT_MS };

        bool connected_a = false, connected_b = false, connected_c = false;

        std::unique_ptr<MockServer> mock_server_a(new MockServer());
        mock_server_a->set_open_handler([&connected_a](websocketpp::connection_hdl hdl) {
            connected_a = true;
        });

        std::unique_ptr<MockServer> mock_server_b(new MockServer());
        mock_server_b->set_open_handler([&connected_b](websocketpp::connection_hdl hdl) {
            connected_b = true;
        });

        auto port_a = mock_server_a->port();
        auto port_b = mock_server_b->port();
        Connection connection {
            std::vector<std::string> { "wss://localhost:" + std::to_string(port_a) + "/pcp",
                                       "wss://localhost:" + std::to_string(port_b) + "/pcp" },
            c_m };

        mock_server_a->go();
        connection.connect(10);
        REQUIRE(connected_a);
        mock_server_a.reset();
        let_connection_stop(connection);

        mock_server_b->go();
        connection.connect(30);
        REQUIRE(connected_b);
        mock_server_b.reset();
        let_connection_stop(connection);

        mock_server_a.reset(new MockServer(port_a));
        mock_server_a->set_open_handler([&connected_c](websocketpp::connection_hdl hdl) {
            connected_c = true;
        });
        mock_server_a->go();
        connection.connect(30);
        REQUIRE(connected_c);
    }
}

TEST_CASE("Connection::~Connection", "[connection]") {
    MockServer mock_server;
    bool connected = false;
    mock_server.set_open_handler([&connected](websocketpp::connection_hdl hdl) {
        connected = true;
    });
    mock_server.go();

    ClientMetadata c_m { "test_client", getCaPath(), getCertPath(),
                         getKeyPath(), WS_TIMEOUT,
                         ASSOCIATION_TIMEOUT_S, ASSOCIATION_REQUEST_TTL_S,
                         PONG_TIMEOUTS_BEFORE_RETRY, PONG_TIMEOUT_MS };

    SECTION("connecting with a single attempt") {
        SECTION("connection timeout = 1 ms") {
            c_m.ws_connection_timeout_ms = 1;
        }

        SECTION("connection timeout = 990 ms") {
            c_m.ws_connection_timeout_ms = 990;
        }

        {
            Connection connection {
                    "wss://localhost:" + std::to_string(mock_server.port()) + "/pcp",
                    c_m };

            // In case the connection timeout fires, this will trigger a
            // WebSocket onFail event
            REQUIRE_NOTHROW(connection.connect(1));

            lth_util::Timer timer {};
            while (connection.getConnectionState() == ConnectionState::connecting
                   && timer.elapsed_seconds() < 2)
                Util::this_thread::sleep_for(Util::chrono::milliseconds(1));

            auto c_s = connection.getConnectionState();
            auto open_or_closed = c_s == ConnectionState::open
                                  || c_s == ConnectionState::closed;
            REQUIRE(open_or_closed);

            // Triggering the dtor
        }

        // Connector's dtor should be able to effectively stop the
        // event handler and join its thread
        REQUIRE(true);
    }
}

}  // namespace PCPClient

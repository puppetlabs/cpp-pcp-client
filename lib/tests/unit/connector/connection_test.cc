#include "tests/test.hpp"
#include "tests/unit/connector/certs.hpp"
#include "tests/unit/connector/mock_server.hpp"

#include <cpp-pcp-client/connector/connection.hpp>
#include <cpp-pcp-client/connector/client_metadata.hpp>
#include <cpp-pcp-client/connector/errors.hpp>
#include <cpp-pcp-client/util/chrono.hpp>
#include <leatherman/util/timer.hpp>
#include <memory>

namespace PCPClient {

namespace lth_util = leatherman::util;

TEST_CASE("Connection::connect errors", "[connector]") {
    SECTION("throws a connection_processing_error if the broker url is "
            "not a valid WebSocket url") {
        ClientMetadata c_m { "test_client", getCaPath(), getCertPath(),
                             getKeyPath(), 6 };
        // NB: the dtor will wait for the 6 ms specified above
        Connection connection { "foo", c_m };

        REQUIRE_THROWS_AS(connection.connect(),
                          connection_processing_error);
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

static constexpr int TEST_TIMEOUT = 100;

TEST_CASE("Connection::connect", "[connector]") {
    SECTION("successfully connects") {
        ClientMetadata c_m { "test_client", getCaPath(), getCertPath(),
                             getKeyPath(), TEST_TIMEOUT };

        MockServer mock_server;
        bool connected = false;
        mock_server.set_open_handler([&connected](websocketpp::connection_hdl hdl) {
            connected = true;
        });
        mock_server.go();

        Connection connection { "wss://localhost:" + std::to_string(mock_server.port()) + "/pcp", c_m };
        connection.connect(10);
        REQUIRE(connected);

        connection.close();
        let_connection_stop(connection);
    }

    SECTION("successfully connects to failover broker") {
        ClientMetadata c_m { "test_client", getCaPath(), getCertPath(),
                             getKeyPath(), TEST_TIMEOUT };

        MockServer mock_server;
        bool connected = false;
        mock_server.set_open_handler([&connected](websocketpp::connection_hdl hdl) {
            connected = true;
        });
        mock_server.go();

        auto port = mock_server.port();
        Connection connection { std::vector<std::string> { "wss://localhost:" + std::to_string(port+1) + "/pcp",
                                                           "wss://localhost:" + std::to_string(port) + "/pcp" }, c_m };
        connection.connect(30);
        REQUIRE(connected);

        connection.close();
        let_connection_stop(connection);
    }

    SECTION("successfully connects to failover when primary broker disappears") {
        ClientMetadata c_m { "test_client", getCaPath(), getCertPath(),
                             getKeyPath(), TEST_TIMEOUT };

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
        Connection connection { std::vector<std::string> { "wss://localhost:" + std::to_string(port_a) + "/pcp",
                                                           "wss://localhost:" + std::to_string(port_b) + "/pcp" }, c_m };

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

}  // namespace PCPClient

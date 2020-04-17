#include "tests/test.hpp"
#include "tests/unit/connector/certs.hpp"
#include "tests/unit/connector/mock_server.hpp"
#include "tests/unit/connector/connector_utils.hpp"

#include <cpp-pcp-client/connector/errors.hpp>
#include <cpp-pcp-client/connector/v2/connector.hpp>
#include <cpp-pcp-client/protocol/v2/schemas.hpp>

#include <memory>
#include <atomic>
#include <functional>

using namespace PCPClient;
using namespace v2;

TEST_CASE("v2::Connector::Connector", "[connector]") {
    SECTION("can instantiate") {
        REQUIRE_NOTHROW(Connector("wss://localhost:8142/pcp", "test_client",
                                  getCaPathCrl(), getGoodCertPathCrl(), getGoodKeyPathCrl(), getEmptyCrlPath(), "",
                                  WS_TIMEOUT_MS,
                                  PONG_TIMEOUTS_BEFORE_RETRY, PONG_TIMEOUT));
    }
}

TEST_CASE("v2::Connector::connect", "[connector]") {
    MockServer mock_server(0, getGoodCertPathCrl(), getGoodKeyPathCrl(), MockServer::Version::v2);
    bool connected = false;
    std::string connection_path;
    mock_server.set_open_handler(
        [&](websocketpp::connection_hdl hdl) {
            connected = true;
            connection_path = mock_server.connection_path(hdl);
        });
    mock_server.go();
    auto port = mock_server.port();
    std::string client_type = "test_client";
    auto server_uri = "wss://localhost:" + std::to_string(port) + "/pcp";

    SECTION("successfully connects and update WebSocket timings") {
        Connector c { server_uri, client_type,
                      getCaPathCrl(), getGoodCertPathCrl(), getGoodKeyPathCrl(), getEmptyCrlPath(), "",
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

    SECTION("successfully connects to broker with client type in the URI") {
        Connector c { server_uri, client_type,
                      getCaPathCrl(), getGoodCertPathCrl(), getGoodKeyPathCrl(), getEmptyCrlPath(), "",
                      WS_TIMEOUT_MS, PONG_TIMEOUTS_BEFORE_RETRY, PONG_TIMEOUT };
        REQUIRE_FALSE(connected);
        REQUIRE_NOTHROW(c.connect(1));

        // NB: ConnectorBase::connect is blocking, but the onOpen handler isn't
        wait_for([&](){return connected;});
        REQUIRE(c.isConnected());
        REQUIRE(connected);
        REQUIRE(connection_path == "/pcp/"+client_type);
    }

    SECTION("successfully connects to broker with trailing slash with client type in the URI") {
        Connector c { server_uri+"/", client_type,
                      getCaPathCrl(), getGoodCertPathCrl(), getGoodKeyPathCrl(), getEmptyCrlPath(), "",
                      WS_TIMEOUT_MS, PONG_TIMEOUTS_BEFORE_RETRY, PONG_TIMEOUT };
        REQUIRE_FALSE(connected);
        REQUIRE_NOTHROW(c.connect(1));

        // NB: ConnectorBase::connect is blocking, but the onOpen handler isn't
        wait_for([&](){return connected;});
        REQUIRE(c.isConnected());
        REQUIRE(connected);
        REQUIRE(connection_path == "/pcp/"+client_type);
    }}

TEST_CASE("v2::Connector::connect revoked-crl", "[connector]") {
    MockServer mock_server(0, getBadCertPathCrl(), getBadKeyPathCrl(), MockServer::Version::v2);
    bool connected = false;
    std::string connection_path;
    mock_server.set_open_handler(
        [&](websocketpp::connection_hdl hdl) {
            connected = true;
            connection_path = mock_server.connection_path(hdl);
        });
    mock_server.go();
    auto port = mock_server.port();
    std::string client_type = "test_client";
    auto server_uri = "wss://localhost:" + std::to_string(port) + "/pcp";

    SECTION("When pcp-broker cert is included in CRL connection is not established") {
        Connector c { server_uri, client_type,
                      getCaPathCrl(), getGoodCertPathCrl(), getGoodKeyPathCrl(), getRevokedCrlPath(), "",
                      WS_TIMEOUT_MS, PONG_TIMEOUTS_BEFORE_RETRY, PONG_TIMEOUT };
        REQUIRE_FALSE(connected);
        REQUIRE_THROWS_AS(c.connect(1), connection_fatal_error);
    }

    SECTION("When invalid CRL is used") {
        Connector c { server_uri, client_type,
                      getCaPathCrl(), getGoodCertPathCrl(), getGoodKeyPathCrl(), getNotACertPath(), "",
                      WS_TIMEOUT_MS, PONG_TIMEOUTS_BEFORE_RETRY, PONG_TIMEOUT };
        REQUIRE_FALSE(connected);
        REQUIRE_THROWS_AS(c.connect(1), connection_config_error);
    }}

TEST_CASE("v2::Connector::send", "[connector]") {
    MockServer mock_server(0, getGoodCertPathCrl(), getGoodKeyPathCrl(), MockServer::Version::v2);
    mock_server.go();
    auto port = mock_server.port();

    SECTION("successfully sends a request and receives a response") {
        Connector c { "wss://localhost:" + std::to_string(port) + "/pcp",
                      "test_client",
                      getCaPathCrl(), getGoodCertPathCrl(), getGoodKeyPathCrl(), getEmptyCrlPath(), "",
                      WS_TIMEOUT_MS,
                      PONG_TIMEOUTS_BEFORE_RETRY, PONG_TIMEOUT };
        std::string response_data, in_reply_to;
        c.registerMessageCallback(Protocol::InventoryResponseSchema(),
            [&](const ParsedChunks& chunks) {
                REQUIRE(chunks.has_data);
                REQUIRE(!chunks.invalid_data);
                response_data = chunks.data.toString();
                REQUIRE(chunks.envelope.includes("in_reply_to"));
                in_reply_to = chunks.envelope.get<std::string>("in_reply_to");
            });
        REQUIRE_NOTHROW(c.connect(1));
        REQUIRE(c.isConnected());

        auto text = "{"
            R"("id":"f0e71a48-969c-4377-b953-35f0fc55c388",)"
            R"("message_type":"http://puppetlabs.com/inventory_request",)"
            R"("data":{"query":["pcp://*/*"]})"
            "}";
        auto id = c.send("pcp:///server", Protocol::INVENTORY_REQ_TYPE, text);
        wait_for([&](){return !in_reply_to.empty();});
        // Response hard-coded in MockServer.
        REQUIRE(response_data == R"({"uris":["pcp://foo/bar"]})");
        REQUIRE(in_reply_to == id);
    }
}

TEST_CASE("v2::Connector::sendError", "[connector]") {
    MockServer mock_server(0, getGoodCertPathCrl(), getGoodKeyPathCrl(), MockServer::Version::v2);
    mock_server.go();
    auto port = mock_server.port();

    SECTION("successfully sends a request and receives a response") {
        Connector c { "wss://localhost:" + std::to_string(port) + "/pcp",
                      "test_client",
                      getCaPathCrl(), getGoodCertPathCrl(), getGoodKeyPathCrl(), getEmptyCrlPath(), "",
                      WS_TIMEOUT_MS,
                      PONG_TIMEOUTS_BEFORE_RETRY, PONG_TIMEOUT };
        std::string response_data, in_reply_to;
        c.setPCPErrorCallback(
            [&](const ParsedChunks& chunks) {
                REQUIRE(chunks.has_data);
                REQUIRE(!chunks.invalid_data);
                response_data = chunks.data.get<std::string>();
                REQUIRE(chunks.envelope.includes("in_reply_to"));
                in_reply_to = chunks.envelope.get<std::string>("in_reply_to");
            });
        REQUIRE_NOTHROW(c.connect(1));
        REQUIRE(c.isConnected());

        auto reply_id = "13131313";
        std::string description = "an error";
        auto id = c.sendError("pcp:///foo/bar", reply_id, description);
        wait_for([&](){return !in_reply_to.empty();});
        REQUIRE(response_data == description);
        REQUIRE(in_reply_to == reply_id);
    }
}

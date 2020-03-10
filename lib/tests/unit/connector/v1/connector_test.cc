#include "tests/test.hpp"
#include "tests/unit/connector/certs.hpp"
#include "tests/unit/connector/mock_server.hpp"
#include "tests/unit/connector/connector_utils.hpp"

#include <cpp-pcp-client/connector/errors.hpp>
#include <cpp-pcp-client/connector/v1/connector.hpp>

#include <memory>
#include <atomic>
#include <functional>

using namespace PCPClient;
using namespace v1;

TEST_CASE("v1::Connector::Connector", "[connector]") {
    SECTION("can instantiate") {
        REQUIRE_NOTHROW(Connector("wss://localhost:8142/pcp", "test_client",
                                  getCaPathCrl(), getGoodCertPathCrl(), getGoodKeyPathCrl(), getEmptyCrlPath(), "",
                                  WS_TIMEOUT_MS,
                                  ASSOCIATION_TIMEOUT_S, ASSOCIATION_REQUEST_TTL_S,
                                  PONG_TIMEOUTS_BEFORE_RETRY, PONG_TIMEOUT));
    }
}

TEST_CASE("v1::Connector::getAssociationTimings", "[connector]") {
    Connector c { "wss://localhost:8142/pcp",
                  "test_client",
                  getCaPathCrl(), getGoodCertPathCrl(), getGoodKeyPathCrl(), getEmptyCrlPath(), "",
                  WS_TIMEOUT_MS, ASSOCIATION_TIMEOUT_S,
                  ASSOCIATION_REQUEST_TTL_S,
                  PONG_TIMEOUTS_BEFORE_RETRY, PONG_TIMEOUT };

    SECTION("can get PCP Association timings") {
        REQUIRE_NOTHROW(c.getAssociationTimings());
    }

    SECTION("timings will say if the session was not associated") {
        auto ass_timings = c.getAssociationTimings();

        REQUIRE_FALSE(ass_timings.completed);
    }
}

TEST_CASE("v1::Connector::connect", "[connector]") {
    SECTION("successfully connects and update WebSocket and Association timings") {
        std::unique_ptr<Connector> c_ptr;
        {
            MockServer mock_server(0, getGoodCertPathCrl(), getGoodKeyPathCrl(), MockServer::Version::v1);
            bool connected = false;
            mock_server.set_open_handler(
                [&connected](websocketpp::connection_hdl hdl) {
                    connected = true;
                });
            mock_server.go();
            auto port = mock_server.port();

            c_ptr.reset(new Connector { "wss://localhost:" + std::to_string(port) + "/pcp",
                                        "test_client",
                                        getCaPathCrl(), getGoodCertPathCrl(), getGoodKeyPathCrl(), getEmptyCrlPath(), "",
                                        WS_TIMEOUT_MS, ASSOCIATION_TIMEOUT_S,
                                        ASSOCIATION_REQUEST_TTL_S,
                                        PONG_TIMEOUTS_BEFORE_RETRY, PONG_TIMEOUT });
            REQUIRE_FALSE(connected);
            REQUIRE_NOTHROW(c_ptr->connect(1));

            // NB: ConnectorBase::connect is blocking, but the onOpen handler isn't
            wait_for([&](){return connected;});
            REQUIRE(c_ptr->isConnected());
            REQUIRE(connected);

            wait_for([&c_ptr](){return c_ptr->isAssociated();});
            auto ws_timings  = c_ptr->getConnectionTimings();
            auto ass_timings = c_ptr->getAssociationTimings();
            auto us_zero     = ConnectionTimings::Duration_us::zero();
            auto min_zero    = ConnectionTimings::Duration_min::zero();

            REQUIRE(c_ptr->isAssociated());
            REQUIRE(ass_timings.completed);
            REQUIRE(ass_timings.success);
            REQUIRE(ass_timings.start < ass_timings.association);
            REQUIRE(us_zero   < ws_timings.getOpeningHandshakeInterval());
            REQUIRE(us_zero   < ws_timings.getWebSocketInterval());
            REQUIRE(us_zero   < ws_timings.getOverallConnectionInterval_us());
            REQUIRE(min_zero == ws_timings.getOverallConnectionInterval_min());
            REQUIRE(us_zero  == ws_timings.getClosingHandshakeInterval());
        }

        // The broker does not exist anymore (RAII)
        wait_for([&c_ptr](){return !c_ptr->isConnected();});
        auto ass_timings = c_ptr->getAssociationTimings();

        REQUIRE_FALSE(c_ptr->isConnected());
        REQUIRE(ass_timings.closed);
        // NB: using timepoints directly as getOverallSessionInterval_min
        //     returns minutes
        REQUIRE(ass_timings.close > ass_timings.start);
    }
    SECTION("When pcp-broker cert is included in CRL connection is not established") {
        std::unique_ptr<Connector> c_ptr;
        {
            MockServer mock_server(0, getBadCertPathCrl(), getBadKeyPathCrl(), MockServer::Version::v1);
            bool connected = false;
            mock_server.set_open_handler(
                [&connected](websocketpp::connection_hdl hdl) {
                    connected = true;
                });
            mock_server.go();
            auto port = mock_server.port();

            c_ptr.reset(new Connector { "wss://localhost:" + std::to_string(port) + "/pcp",
                                        "test_client",
                                        getCaPathCrl(), getGoodCertPathCrl(), getGoodKeyPathCrl(), getRevokedCrlPath(), "",
                                        WS_TIMEOUT_MS, ASSOCIATION_TIMEOUT_S,
                                        ASSOCIATION_REQUEST_TTL_S,
                                        PONG_TIMEOUTS_BEFORE_RETRY, PONG_TIMEOUT });
            REQUIRE_FALSE(connected);
            REQUIRE_THROWS_AS(c_ptr->connect(1), connection_fatal_error);
        }
    }
}

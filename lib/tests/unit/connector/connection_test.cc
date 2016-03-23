#include "tests/test.hpp"
#include "tests/unit/connector/certs.hpp"

#include <cpp-pcp-client/connector/connection.hpp>
#include <cpp-pcp-client/connector/client_metadata.hpp>
#include <cpp-pcp-client/connector/errors.hpp>

namespace PCPClient {

TEST_CASE("Connection::connect", "[connector]") {
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

}  // namespace PCPClient

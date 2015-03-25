#include "tests/test.hpp"
#include "tests/unit/connector/certs.hpp"

#include <cthun-client/connector/connection.hpp>
#include <cthun-client/connector/client_metadata.hpp>
#include <cthun-client/connector/errors.hpp>

namespace CthunClient {

TEST_CASE("Connection::connect", "[connector]") {
    SECTION("throws a connection_processing_error if the server url is "
            "not a valid WebSocket url") {
        ClientMetadata c_m { "test_client", getCaPath(), getCertPath(),
                             getKeyPath() };
        Connection connection { "foo", c_m };

        REQUIRE_THROWS_AS(connection.connect(),
                          connection_processing_error);
    }
}

}  // namespace CthunClient

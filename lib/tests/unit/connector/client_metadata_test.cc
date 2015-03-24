#include "tests/test.h"
#include "certs.h"

#include <cthun-client/connector/client_metadata.h>
#include <cthun-client/connector/errors.h>

#include <string>

namespace CthunClient {

TEST_CASE("ClientMetadata::ClientMetadata", "[connector]") {
    SECTION("retrieves correctly the client identity from the certificate") {
        std::string type { "test" };
        ClientMetadata c_m { type, getCaPath(), getCertPath(), getKeyPath() };
        std::string expected_id { "cth://cthun-client/" + type };

        REQUIRE(c_m.id == expected_id);
    }

    SECTION("throws a connection_config_error if the provided certificate "
            "file does not exist") {
        REQUIRE_THROWS_AS(ClientMetadata("test", getCaPath(),
                                         getNotExistentFilePath(), getKeyPath()),
                          connection_config_error);
    }

    SECTION("throws a connection_config_error if the provided certificate "
            "is invalid") {
        REQUIRE_THROWS_AS(ClientMetadata("test", getCaPath(), getNotACertPath(),
                                         getKeyPath()),
                          connection_config_error);
    }
}

}  // namespace CthunClient

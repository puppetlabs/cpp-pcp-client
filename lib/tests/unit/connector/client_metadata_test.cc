#include "tests/test.hpp"
#include "certs.hpp"

#include <cpp-pcp-client/connector/client_metadata.hpp>
#include <cpp-pcp-client/connector/errors.hpp>

#include <string>

namespace PCPClient {

// TODO(ale): update cert identities so we don't match against cthun

TEST_CASE("ClientMetadata::ClientMetadata", "[connector]") {
    SECTION("retrieves correctly the client common name from the certificate") {
        std::string type { "test" };
        ClientMetadata c_m { type, getCaPath(), getCertPath(), getKeyPath() };
        std::string expected_id { "pcp://cthun-client/" + type };

        REQUIRE(c_m.common_name == "cthun-client");
    }

    SECTION("determines correctly the client URI") {
        std::string type { "test" };
        ClientMetadata c_m { type, getCaPath(), getCertPath(), getKeyPath() };
        std::string expected_uri { "pcp://cthun-client/" + type };

        REQUIRE(c_m.uri == expected_uri);
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

}  // namespace PCPClient

#include "tests/test.hpp"
#include "certs.hpp"

#include <cpp-pcp-client/connector/client_metadata.hpp>
#include <cpp-pcp-client/connector/errors.hpp>

#include <string>

namespace PCPClient {

TEST_CASE("validatePrivateKeyCertPair", "[connector]") {
    SECTION("validates a matched key cert pair") {
        REQUIRE_NOTHROW(validatePrivateKeyCertPair(getKeyPath(), getCertPath()));
    }

    SECTION("does not validate a mismatched key cert pair") {
        REQUIRE_THROWS_AS(validatePrivateKeyCertPair(getMismatchedKeyPath(),
                                                     getCertPath()),
                          connection_config_error);
    }

    SECTION("raises a connection_config_error with no CL prompt in case of "
            "password protected key") {
        REQUIRE_THROWS_AS(validatePrivateKeyCertPair(getProtectedKeyPath(),
                                                     getCertPath()),
                          connection_config_error);
    }
}

TEST_CASE("ClientMetadata::ClientMetadata", "[connector]") {
    SECTION("retrieves correctly the client common name from the certificate") {
        std::string type { "test" };
        ClientMetadata c_m { type, getCaPath(), getCertPath(), getKeyPath(), 5000 };
        std::string expected_id { "pcp://cthun-client/" + type };

        REQUIRE(c_m.common_name == "cthun-client");
    }

    SECTION("determines correctly the client URI") {
        std::string type { "test" };
        ClientMetadata c_m { type, getCaPath(), getCertPath(), getKeyPath(), 5000 };
        std::string expected_uri { "pcp://cthun-client/" + type };

        REQUIRE(c_m.uri == expected_uri);
    }

    SECTION("throws a connection_config_error if the provided certificate "
            "file does not exist") {
        REQUIRE_THROWS_AS(ClientMetadata("test", getCaPath(),
                                         getNotExistentFilePath(), getKeyPath(),
                                         5000),
                          connection_config_error);
    }

    SECTION("throws a connection_config_error if the provided certificate "
            "is invalid") {
        REQUIRE_THROWS_AS(ClientMetadata("test", getCaPath(), getNotACertPath(),
                                         getKeyPath(), 5000),
                          connection_config_error);
    }
}

}  // namespace PCPClient

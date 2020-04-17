#include "tests/test.hpp"
#include "certs.hpp"

#include <cpp-pcp-client/connector/client_metadata.hpp>
#include <cpp-pcp-client/connector/errors.hpp>

#include <string>

using namespace PCPClient;

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

static constexpr int WS_TIMEOUT { 5000 };
static constexpr uint32_t ASSOCIATION_TIMEOUT_S { 15 };
static constexpr uint32_t ASSOCIATION_REQUEST_TTL_S { 10 };
static constexpr uint32_t PONG_TIMEOUTS_BEFORE_RETRY { 3 };
static constexpr uint32_t PONG_TIMEOUT_MS { 10000 };

TEST_CASE("ClientMetadata::ClientMetadata", "[connector]") {
    SECTION("retrieves correctly the client common name from the certificate (call with proxy, crl constructor)") {
        std::string type { "test" };
        ClientMetadata c_m { type, getCaPath(), getCertPath(), getKeyPath(), getEmptyCrlPath(), "",
                             WS_TIMEOUT, PONG_TIMEOUTS_BEFORE_RETRY, PONG_TIMEOUT_MS };

        REQUIRE(c_m.common_name == "localhost");
    }

    SECTION("determines correctly the client URI") {
        std::string type { "test" };
        ClientMetadata c_m { type, getCaPath(), getCertPath(), getKeyPath(),
                             WS_TIMEOUT, PONG_TIMEOUTS_BEFORE_RETRY, PONG_TIMEOUT_MS };
        std::string expected_uri { "pcp://localhost/" + type };

        REQUIRE(c_m.uri == expected_uri);
    }

    SECTION("throws a connection_config_error if the provided certificate "
            "file does not exist") {
        REQUIRE_THROWS_AS(ClientMetadata("test", getCaPath(),
                                         getNotExistentFilePath(), getKeyPath(),
                                         WS_TIMEOUT, PONG_TIMEOUTS_BEFORE_RETRY, PONG_TIMEOUT_MS),
                          connection_config_error);
    }

    SECTION("throws a connection_config_error if the provided certificate "
            "is invalid") {
        REQUIRE_THROWS_AS(ClientMetadata("test", getCaPath(), getNotACertPath(),
                                         getKeyPath(),
                                         WS_TIMEOUT, PONG_TIMEOUTS_BEFORE_RETRY, PONG_TIMEOUT_MS),
                          connection_config_error);
    }
}

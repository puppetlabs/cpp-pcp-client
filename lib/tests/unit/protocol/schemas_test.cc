#include "tests/test.hpp"

#include <cthun-client/protocol/schemas.hpp>

namespace CthunClient {

TEST_CASE("getAssociateResponseSchema", "[message]") {
    REQUIRE_NOTHROW(Protocol::getAssociateResponseSchema());
}

}  // namespace CthunClient

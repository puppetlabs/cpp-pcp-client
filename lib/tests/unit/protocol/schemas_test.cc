#include "tests/test.hpp"

#include <cthun-client/protocol/schemas.hpp>

namespace CthunClient {

TEST_CASE("AssociateResponseSchema", "[message]") {
    REQUIRE_NOTHROW(Protocol::AssociateResponseSchema());
}

}  // namespace CthunClient

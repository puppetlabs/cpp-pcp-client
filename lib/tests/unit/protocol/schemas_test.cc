#include "tests/test.hpp"

#include <cpp-pcp-client/protocol/schemas.hpp>

namespace CthunClient {

TEST_CASE("AssociateResponseSchema", "[message]") {
    REQUIRE_NOTHROW(Protocol::AssociateResponseSchema());
}

}  // namespace CthunClient

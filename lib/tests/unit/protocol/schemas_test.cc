#include "tests/test.hpp"

#include <cpp-pcp-client/protocol/schemas.hpp>

namespace PCPClient {

TEST_CASE("AssociateResponseSchema", "[message]") {
    REQUIRE_NOTHROW(Protocol::AssociateResponseSchema());
}

}  // namespace PCPClient

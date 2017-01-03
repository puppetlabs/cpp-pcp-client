#include "tests/test.hpp"

#include <cpp-pcp-client/protocol/v1/schemas.hpp>

namespace PCPClient {
using namespace v1;

TEST_CASE("AssociateResponseSchema", "[message]") {
    REQUIRE_NOTHROW(Protocol::AssociateResponseSchema());
}

}  // namespace PCPClient

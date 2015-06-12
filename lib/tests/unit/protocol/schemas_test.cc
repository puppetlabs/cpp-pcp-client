#include "tests/test.hpp"

#include <cthun-client/protocol/schemas.hpp>

namespace CthunClient {

TEST_CASE("AssociateResponseSchema", "[message]") {
    REQUIRE_NOTHROW(Protocol::AssociateResponseSchema());
}

TEST_CASE("AssociateResponseSchema name", "[message]") {
    auto a_r_sch = Protocol::AssociateResponseSchema();

    REQUIRE(a_r_sch.getName() == Protocol::ASSOCIATE_RESP_TYPE);
}

}  // namespace CthunClient

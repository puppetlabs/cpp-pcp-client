#include "test/test.h"

#include "src/validator/validator.h"
#include "src/validator/schema.h"
#include "src/data_container/data_container.h"

namespace CthunClient {

TEST_CASE("Validator::registerSchema", "[registerSchema]") {
    Validator validator {};
    Schema schema {};

    SECTION("it can register a schema") {
        validator.registerSchema("test-schema", schema);
    }

    SECTION("it cannot register a schema name more than once") {
        validator.registerSchema("test-schema", schema);
        REQUIRE_THROWS_AS(validator.registerSchema("test-schema", schema),
                          schema_redefinition_error);
    }
}

TEST_CASE("Validator::validate", "[validate]") {
    DataContainer data {};
    Schema schema {};
    Validator validator {};

    SECTION("it throws on an invalid schema") {
        REQUIRE_THROWS_AS(validator.validate(data, "test-schema"),
                          validation_error);
    }

    SECTION("it throws when validation fails") {
        data.set<std::string>("key", "value");
        schema.addConstraint("key", TypeConstraint::Int);
        validator.registerSchema("test-schema", schema);
        REQUIRE_THROWS_AS(validator.validate(data, "test-schema"),
                          validation_error);
    }

    SECTION("it doesn't throw when validation succeeds") {
        data.set<std::string>("key", "value");
        schema.addConstraint("key", TypeConstraint::String);
        validator.registerSchema("test-schema", schema);
        REQUIRE_NOTHROW(validator.validate(data, "test-schema"));
    }

    SECTION("default schemas") {
        SECTION("it can validate a correctly formed envelope schema") {
            data.set<std::string>("id", "1234");
            data.set<std::string>("expires", "later");
            data.set<std::string>("sender", "test_sender");
            std::vector<std::string> endpoints {};
            data.set<std::vector<std::string>>("endpoints", endpoints);
            data.set<std::string>("data_schema", "envelope");
            data.set<bool>("destination_report", true);
            REQUIRE_NOTHROW(validator.validate(data, "envelope"));
        }

        SECTION("it can validate an incorrectly formed envelope schema") {
            data.set<std::string>("id", "1234");
        REQUIRE_THROWS_AS(validator.validate(data, "envelope"),
                          validation_error);}
    }
}

TEST_CASE("Validator::getSchemaContentType", "[getSchemaContentType]") {
    Schema schema { ContentType::Binary };
    Validator validator {};

    SECTION("it can return the schema's content type") {
        validator.registerSchema("test-schema", schema);
        REQUIRE(validator.getSchemaContentType("test-schema") ==
                ContentType::Binary);
    }

    SECTION("it throws when the schema is undefined") {
        REQUIRE_THROWS_AS(validator.getSchemaContentType("test-schema"),
                         schema_not_found_error);
    }
}


}  // namespace CthunClient

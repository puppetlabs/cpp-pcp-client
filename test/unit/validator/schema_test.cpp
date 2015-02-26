#include "test/test.h"

#include "src/validator/schema.h"
#include "src/data_container/data_container.h"

#include <valijson/adapters/rapidjson_adapter.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validation_results.hpp>
#include <valijson/validator.hpp>

namespace CthunClient {

bool validate_test(DataContainer document, Schema schema) {
    valijson::Validator validator { schema.getRaw() };
    valijson::adapters::RapidJsonAdapter adapted_document { document.getRaw() };
    valijson::ValidationResults validation_results;

    return validator.validate(adapted_document, &validation_results);
}

TEST_CASE("Schema::addConstraint(type)", "[addConstraint]") {
    Schema schema {};

    SECTION("it creates an interger constraint") {
        DataContainer data { "{\"foo\" : 2}" };
        schema.addConstraint("foo", TypeConstraint::Int, true);
        REQUIRE(validate_test(data, schema));
    }

    SECTION("it creates an string constraint") {
        DataContainer data { "{\"foo\" : \"bar\"}" };
        schema.addConstraint("foo", TypeConstraint::String, true);
        REQUIRE(validate_test(data, schema));
    }

    SECTION("it creates an double constraint") {
        DataContainer data { "{\"foo\" : 0.0 }" };
        schema.addConstraint("foo", TypeConstraint::Double, true);
        REQUIRE(validate_test(data, schema));
    }

    SECTION("it creates an boolean constraint") {
        DataContainer data { "{\"foo\" : true }" };
        schema.addConstraint("foo", TypeConstraint::Bool, true);
        REQUIRE(validate_test(data, schema));
    }

    SECTION("it creates an array constraint") {
        DataContainer data { "{\"foo\" : []}" };
        schema.addConstraint("foo", TypeConstraint::Array, true);
        REQUIRE(validate_test(data, schema));
    }

    SECTION("it creates an string constraint") {
        DataContainer data { "{\"foo\" : {}}" };
        schema.addConstraint("foo", TypeConstraint::Object, true);
        REQUIRE(validate_test(data, schema));
    }

    SECTION("it creates an string constraint") {
        DataContainer data { "{\"foo\" : null}" };
        schema.addConstraint("foo", TypeConstraint::Null, true);
        REQUIRE(validate_test(data, schema));
    }

    SECTION("it creates an any constraint") {
        DataContainer data { "{\"foo\" : 1}" };
        schema.addConstraint("foo", TypeConstraint::Any, true);
        REQUIRE(validate_test(data, schema));
    }

   SECTION("it creates optional constraint") {
        DataContainer data { "{}" };
        schema.addConstraint("foo", TypeConstraint::Int, false);
        REQUIRE(validate_test(data, schema));
    }

    SECTION("it creates multiple constraint") {
        DataContainer data { "{\"foo\" : \"bar\","
                             "\"baz\" : 1 }" };
        schema.addConstraint("foo", TypeConstraint::String, true);
        schema.addConstraint("baz", TypeConstraint::Int, true);
        REQUIRE(validate_test(data, schema));
    }

    SECTION("it can be modified after use") {
        DataContainer data { "{\"foo\" : \"bar\"}" };
        schema.addConstraint("foo", TypeConstraint::String, true);
        validate_test(data, schema);
        schema.addConstraint("baz", TypeConstraint::Int, true);
        DataContainer data2 { "{\"foo\" : \"bar\","
                              "\"baz\" : 1 }" };
        REQUIRE(!validate_test(data, schema));
        REQUIRE(validate_test(data2, schema));
    }
}

TEST_CASE("Schema::addConstraint(subschema)", "[addConstraint]") {
    Schema schema {};
    SECTION("it creates a sub schema constraint") {
        DataContainer data {"{\"root\" : "
                                "{\"foo\" : \"bar\","
                                "\"baz\" : 1 }}" };
        Schema subschema {};
        subschema.addConstraint("foo", TypeConstraint::String, true);
        subschema.addConstraint("baz", TypeConstraint::Int, true);
        schema.addConstraint("root", subschema, true);
        REQUIRE(validate_test(data, schema));
    }
}

}  // namespace CthunClient

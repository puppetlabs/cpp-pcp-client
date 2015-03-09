#include "schema.h"

#include <valijson/schema_parser.hpp>
#include <valijson/adapters/rapidjson_adapter.hpp>

#include <rapidjson/allocators.h>

namespace CthunClient {

//
// Free functions
//

valijson::Schema parseSchema(DataContainer& metadata) {
    valijson::Schema schema {};
    valijson::SchemaParser parser {};
    valijson::adapters::RapidJsonAdapter r_j_schema { metadata.getRaw() };

    parser.populateSchema(r_j_schema, schema);
    return schema;
}

//
// Public API
//

Schema::Schema(const std::string& name, ContentType content_type)
        : name_ { name },
          content_type_ { content_type },
          parsed_json_schema_ {},
          parsed_ { false } {
}

Schema::Schema(const std::string& name)
        : Schema(name, ContentType::Json) {
}

const Schema::Schema(const std::string& name, DataContainer metadata)
        try : name_ { name },
              content_type_ { ContentType::Json },
              parsed_json_schema_ { parseSchema(metadata) },
              parsed_ { true } {
} catch (std::exception& e) {
    throw schema_error { std::string("failed to parse schema: ") + e.what() };
} catch (...) {
    throw schema_error { "failed to parse schema" };
}

void Schema::addConstraint(std::string field, TypeConstraint type, bool required) {
    if (parsed_) {
        throw schema_error { "cannot add constraints to a schema that "
                             "was previously parsed" };
    }

    V_C::TypeConstraint constraint { getConstraint(type) };

    // Add optional type constraint
    properties_[field].addConstraint(constraint);

    if (required) {
        // add required constraint
        required_properties_.insert(field);
    }
}

void Schema::addConstraint(std::string field, Schema sub_schema, bool required) {
    if (parsed_) {
        throw schema_error { "cannot add constraints to a schema that "
                             "was previously parsed" };
    }

    V_C::ItemsConstraint sub_schema_constraint { sub_schema.getRaw() };

    // Add optional schema constraint
    properties_[field].addConstraint(sub_schema_constraint);

    if (required) {
        required_properties_.insert(field);
    }
}

const std::string Schema::getName() const {
    return name_;
}

ContentType Schema::getContentType() const {
    return content_type_;
}

const valijson::Schema Schema::getRaw() const {
    if (parsed_) {
        return parsed_json_schema_;
    }

    valijson::Schema schema {};
    auto constraint = getConstraint(TypeConstraint::Object);
    schema.addConstraint(constraint);

    if (!properties_.empty()) {
        schema.addConstraint(new V_C::PropertiesConstraint(properties_,
                                                           pattern_properties_));
    }

    if (!required_properties_.empty()) {
        schema.addConstraint(new V_C::RequiredConstraint(required_properties_));
    }

    return schema;
}

//
// Private methods
//

V_C::TypeConstraint Schema::getConstraint(TypeConstraint type) const {
    switch (type) {
        case TypeConstraint::Object :
            return V_C::TypeConstraint::kObject;
        case TypeConstraint::Array :
            return V_C::TypeConstraint::kArray;
        case TypeConstraint::String :
            return V_C::TypeConstraint::kString;
        case TypeConstraint::Int :
            return V_C::TypeConstraint::kInteger;
        case TypeConstraint::Bool :
            return V_C::TypeConstraint::kBoolean;
        case TypeConstraint::Double :
            return V_C::TypeConstraint::kNumber;
        case TypeConstraint::Null :
            return V_C::TypeConstraint::kNull;
        case TypeConstraint::Any :
            return V_C::TypeConstraint::kAny;
    }
}

}  // namespace CthunClient

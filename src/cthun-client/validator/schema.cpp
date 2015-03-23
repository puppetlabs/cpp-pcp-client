#include "schema.h"

#include <valijson/schema_parser.hpp>
#include <valijson/adapters/rapidjson_adapter.hpp>

#include <rapidjson/allocators.h>

namespace CthunClient {

//
// Free functions
//

valijson::Schema parseSchema(const DataContainer& metadata) {
    valijson::Schema schema {};
    valijson::SchemaParser parser {};
    valijson::adapters::RapidJsonAdapter r_j_schema { metadata.getRaw() };

    parser.populateSchema(r_j_schema, schema);
    return schema;
}

//
// Public API
//

Schema::Schema(const std::string& name,
               const ContentType content_type,
               const TypeConstraint type)
        : name_ { name },
          content_type_ { content_type },
          parsed_json_schema_ { new valijson::Schema() },
          parsed_ { false },
          type_ { type },
          properties_ { new V_C::PropertiesConstraint::PropertySchemaMap() },
          pattern_properties_ { new V_C::PropertiesConstraint::PropertySchemaMap() },
          required_properties_ { new V_C::RequiredConstraint::RequiredProperties() } {
}

Schema::Schema(const std::string& name,
               const ContentType content_type)
        : Schema(name, content_type, TypeConstraint::Object) {
}

Schema::Schema(const std::string& name,
               const TypeConstraint type)
        : Schema(name, ContentType::Json, type) {
}

Schema::Schema(const std::string& name)
        : Schema(name, ContentType::Json, TypeConstraint::Object) {
}

Schema::Schema(const Schema& s)
        : name_ { s.name_ },
          content_type_ { s.content_type_ },
          type_ { s.type_ },
          parsed_json_schema_ { new valijson::Schema(*s.parsed_json_schema_) },
          parsed_ { s.parsed_ },
          properties_ {
            new V_C::PropertiesConstraint::PropertySchemaMap(*s.properties_) },
          pattern_properties_ {
            new V_C::PropertiesConstraint::PropertySchemaMap(*s.pattern_properties_) },
          required_properties_ {
            new V_C::RequiredConstraint::RequiredProperties(*s.required_properties_)} {
}

Schema::Schema(const std::string& name, const DataContainer metadata)
        try : name_ { name },
              content_type_ { ContentType::Json },
              parsed_json_schema_ { new valijson::Schema(parseSchema(metadata)) },
              parsed_ { true },
              type_ { TypeConstraint::Object },
              properties_ { new V_C::PropertiesConstraint::PropertySchemaMap() },
              pattern_properties_ { new V_C::PropertiesConstraint::PropertySchemaMap() },
              required_properties_ { new V_C::RequiredConstraint::RequiredProperties() } {
} catch (std::exception& e) {
    throw schema_error { std::string("failed to parse schema: ") + e.what() };
} catch (...) {
    throw schema_error { "failed to parse schema" };
}

// unique_ptr requires a complete type at time of destruction. this forces us to
// either have an empty destructor or use a shared_ptr instead.
Schema::~Schema() {}

void Schema::addConstraint(std::string field, TypeConstraint type, bool required) {
    checkAddConstraint();

    V_C::TypeConstraint constraint { getConstraint(type) };

    // Add optional type constraint
    (*properties_)[field].addConstraint(constraint);

    if (required) {
        // add required constraint
        required_properties_->insert(field);
    }
}

void Schema::addConstraint(std::string field, Schema sub_schema, bool required) {
    checkAddConstraint();

    V_C::ItemsConstraint sub_schema_constraint { sub_schema.getRaw() };

    // Add optional schema constraint
    (*properties_)[field].addConstraint(sub_schema_constraint);

    if (required) {
        required_properties_->insert(field);
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
        return *parsed_json_schema_;
    }

    valijson::Schema schema {};
    auto constraint = getConstraint(type_);
    schema.addConstraint(constraint);

    if (!properties_->empty()) {
        schema.addConstraint(new V_C::PropertiesConstraint(*properties_,
                                                           *pattern_properties_));
    }

    if (!required_properties_->empty()) {
        schema.addConstraint(new V_C::RequiredConstraint(*required_properties_));
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
        default:
            return V_C::TypeConstraint::kAny;
    }
}

void Schema::checkAddConstraint() {
    if (parsed_) {
        throw schema_error { "schema was populate by parsing JSON" };
    }

    if (type_ != TypeConstraint::Object) {
        throw schema_error { "type is not JSON Object" };
    }
}

}  // namespace CthunClient

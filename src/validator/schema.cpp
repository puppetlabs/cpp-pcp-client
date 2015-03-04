#include "schema.h"

#include <valijson/schema_parser.hpp>

namespace CthunClient {

///
/// Public API
///

Schema::Schema(const std::string& name, ContentType content_type)
        : name_ { name },
          content_type_ { content_type } {
}

Schema::Schema(const std::string& name)
        : Schema(name, ContentType::Json) {
}

void Schema::addConstraint(std::string field, TypeConstraint type, bool required) {
    V_C::TypeConstraint constraint { getConstraint(type) };

    // Add optional type constraint
    properties_[field].addConstraint(constraint);

    if (required) {
        // add required constraint
        required_properties_.insert(field);
    }
}

void Schema::addConstraint(std::string field, Schema sub_schema, bool required) {
    V_C::ItemsConstraint sub_schema_constraint { sub_schema.getRaw() };

    // Add optional schema constraint
    properties_[field].addConstraint(sub_schema_constraint);

    if (required) {
        required_properties_.insert(field);
    }
}

ContentType Schema::getContentType() const {
    return content_type_;
}

const valijson::Schema Schema::getRaw() const {
    valijson::Schema schema;

    V_C::TypeConstraint constraint { getConstraint(TypeConstraint::Object) };
    schema.addConstraint(constraint);
    schema.addConstraint(new V_C::PropertiesConstraint(properties_,
                                                       pattern_properties_));
    schema.addConstraint(new V_C::RequiredConstraint(required_properties_));
    return schema;
}

const std::string Schema::getName() const {
    return name_;
}

///
/// Private methods
///

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

#ifndef CTHUN_CLIENT_SRC_VALIDATOR_SCHEMA_H_
#define CTHUN_CLIENT_SRC_VALIDATOR_SCHEMA_H_

#include <valijson/schema_parser.hpp>
#include <map>

namespace CthunClient {

namespace V_C = valijson::constraints;

enum class TypeConstraint { Object, Array, String, Int, Bool, Double, Null, Any };

class Schema {
  public:
    Schema() {};
    void addConstraint(std::string field, TypeConstraint type, bool required = false);
    void addConstraint(std::string field, Schema sub_schema, bool required = false);
    const valijson::Schema getRaw() const;

  private:
    V_C::PropertiesConstraint::PropertySchemaMap properties_;
    V_C::PropertiesConstraint::PropertySchemaMap pattern_properties_;
    V_C::RequiredConstraint::RequiredProperties required_properties_;

    V_C::TypeConstraint getConstraint(TypeConstraint type) const;
};

}  // namespace CthunClient

#endif // CTHUN_CLIENT_SRC_VALIDATOR_SCHEMA_H_

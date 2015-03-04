#ifndef CTHUN_CLIENT_SRC_VALIDATOR_SCHEMA_H_
#define CTHUN_CLIENT_SRC_VALIDATOR_SCHEMA_H_

#include <valijson/schema_parser.hpp>
#include <map>
#include <string>

namespace CthunClient {

namespace V_C = valijson::constraints;

enum class TypeConstraint { Object, Array, String, Int, Bool, Double, Null, Any };
enum class ContentType { Json, Binary };

class Schema {
  public:
    Schema() = delete;
    Schema(const std::string& name, ContentType content_type);
    Schema(const std::string& name);

    void addConstraint(std::string field, TypeConstraint type, bool required = false);
    void addConstraint(std::string field, Schema sub_schema, bool required = false);
    ContentType getContentType() const;
    const valijson::Schema getRaw() const;
    const std::string getName() const;

  private:
    std::string name_;
    ContentType content_type_;
    V_C::PropertiesConstraint::PropertySchemaMap properties_;
    V_C::PropertiesConstraint::PropertySchemaMap pattern_properties_;
    V_C::RequiredConstraint::RequiredProperties required_properties_;

    V_C::TypeConstraint getConstraint(TypeConstraint type) const;
    void copy_(const Schema& other_schema);
};

}  // namespace CthunClient

#endif  // CTHUN_CLIENT_SRC_VALIDATOR_SCHEMA_H_

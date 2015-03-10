#ifndef CTHUN_CLIENT_SRC_VALIDATOR_SCHEMA_H_
#define CTHUN_CLIENT_SRC_VALIDATOR_SCHEMA_H_

#include "../data_container/data_container.h"

#include <valijson/schema_parser.hpp>

#include <map>
#include <string>

namespace CthunClient {

namespace V_C = valijson::constraints;

enum class TypeConstraint { Object, Array, String, Int, Bool, Double, Null, Any };
enum class ContentType { Json, Binary };

class schema_error : public std::runtime_error  {
  public:
    explicit schema_error(std::string const& msg)
            : std::runtime_error(msg) {}
};

class Schema {
  public:
    Schema() = delete;

    // The following constructors instantiate an empty Schema with no
    // constraint

    Schema(const std::string& name,
           const ContentType content_type,
           const TypeConstraint type);

    Schema(const std::string& name,
           const ContentType content_type);

    Schema(const std::string& name,
           const TypeConstraint type);

    Schema(const std::string& name);

    // Instantiate a Schema of type ContentType::Json by parsing the
    // JSON schema passed as a DataContainer object.
    // It won't be possible to add further constraints to such schema
    // (hence the const modifier, even though ineffective).
    // Throw a schema_error in case of parsing failure.
    const Schema(const std::string& name,
                 const DataContainer json_schema);

    // Add constraints to a JSON object schema.
    // Throw a schema_error in case the Schema instance is not of
    // TypeConstraint::Object type or if it was constructed by parsing
    // a JSON schema.
    void addConstraint(std::string field, TypeConstraint type, bool required = false);
    void addConstraint(std::string field, Schema sub_schema, bool required = false);

    const std::string getName() const;
    ContentType getContentType() const;
    const valijson::Schema getRaw() const;

  private:
    std::string name_;

    // To distinguish between binary and JSON data
    const ContentType content_type_;

    // To add a global type constraint; default to Object (see ctors)
    const TypeConstraint type_;

    // Stores the schema parsed by the parsing ctor
    const valijson::Schema parsed_json_schema_;

    // Flag; set in case the used ctor was the parsing one
    const bool parsed_;

    // Store constraints for validjson
    V_C::PropertiesConstraint::PropertySchemaMap properties_;
    V_C::PropertiesConstraint::PropertySchemaMap pattern_properties_;
    V_C::RequiredConstraint::RequiredProperties required_properties_;

    // Convert CthunClient::TypeConstraint to the validjson ones
    V_C::TypeConstraint getConstraint(TypeConstraint type) const;

    // Check if it's possible to add constraints
    void checkAddConstraint();
};

}  // namespace CthunClient

#endif  // CTHUN_CLIENT_SRC_VALIDATOR_SCHEMA_H_

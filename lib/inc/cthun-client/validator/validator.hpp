#ifndef CTHUN_CLIENT_SRC_VALIDATOR_VALIDATOR_H_
#define CTHUN_CLIENT_SRC_VALIDATOR_VALIDATOR_H_

#include <cthun-client/validator/schema.hpp>
#include <cthun-client/data_container/data_container.hpp>

#include <map>
#include <mutex>

namespace CthunClient {

//
// Errors
//

/// General validator error
class validator_error : public std::runtime_error  {
  public:
    explicit validator_error(std::string const& msg)
        : std::runtime_error(msg) {}
};

class schema_redefinition_error : public validator_error  {
  public:
    explicit schema_redefinition_error(std::string const& msg)
        : validator_error(msg) {}
};

class schema_not_found_error : public validator_error  {
  public:
    explicit schema_not_found_error(std::string const& msg)
        : validator_error(msg) {}
};

class validation_error : public validator_error {
  public:
    explicit validation_error(std::string const& msg)
        : validator_error(msg) {}
};

//
// Validator
//

class Validator {
  public:
    Validator();

    // NB: Validator is thread-safe; it employs a mutex for that. As a
    //     consequence, Validator instances are not copyable.
    Validator(Validator&& other_validator);

    void registerSchema(const Schema& schema);

    // Validates data with the specified schema.
    // Throw a schema_not_found error in case the specified schema
    // was not registered.
    // Throw a validation_error in case the data does not match the
    // specified schema.
    void validate(DataContainer& data, std::string schema_name) const;

    bool includesSchema(std::string schema_name) const;
    ContentType getSchemaContentType(std::string schema_name) const;

  private:
    std::map<std::string, Schema> schema_map_;
    mutable std::mutex lookup_mutex_;
};

}  // namespace CthunClient

#endif  // CTHUN_CLIENT_SRC_VALIDATOR_VALIDATOR_H_

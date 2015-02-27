#ifndef CTHUN_CLIENT_SRC_VALIDATOR_VALIDATOR_H_
#define CTHUN_CLIENT_SRC_VALIDATOR_VALIDATOR_H_

#include "schema.h"
#include "../data_container/data_container.h"

#include <map>
#include <mutex>

namespace CthunClient {

// Errors

/// General validator error
class validator_error : public std::runtime_error  {
  public:
    explicit validator_error(std::string const& msg) : std::runtime_error(msg) {}
};

class schema_redefinition_error : public validator_error  {
  public:
    explicit schema_redefinition_error(std::string const& msg) : validator_error(msg) {}
};

class schema_not_found_error : public validator_error  {
  public:
    explicit schema_not_found_error(std::string const& msg) : validator_error(msg) {}
};

class validation_error : public validator_error {
  public:
    explicit validation_error(std::string const& msg) : validator_error(msg) {}
};

class Validator {
  public:
    Validator();
    void registerSchema(const std::string& schema_name, const Schema& schema);
    void validate(DataContainer& data, std::string schema_name) const;
    bool includesSchema(std::string schema_name) const;
    ContentType getSchemaContentType(std::string schema_name) const;

  private:
    std::map<std::string, Schema> schema_map_;
    mutable std::mutex lookup_mutex_;

    void registerDefaultSchemas();
    bool validateDataContainer(DataContainer& data, const Schema& schema) const;
};

}  // namespace CthunClient

#endif // CTHUN_CLIENT_SRC_VALIDATOR_VALIDATOR_H_

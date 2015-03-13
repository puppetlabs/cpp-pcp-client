#include "validator.h"

#include <valijson/adapters/rapidjson_adapter.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validation_results.hpp>
#include <valijson/validator.hpp>

namespace CthunClient {


///
/// Public API
///

Validator::Validator()
        : schema_map_ {},
          lookup_mutex_ {} {
}

Validator::Validator(Validator&& other_validator)
        : schema_map_ { other_validator.schema_map_ },
          lookup_mutex_ {} {
}

void Validator::registerSchema(const Schema& schema) {
    std::lock_guard<std::mutex> lock(lookup_mutex_);
    auto schema_name = schema.getName();
    if (includesSchema(schema_name)) {
        throw schema_redefinition_error { "Schema '" + schema_name +
                                          "' already defined." };
    }

    auto p = std::pair<std::string, Schema>(schema_name, schema);
    schema_map_.insert(p);
}

void Validator::validate(DataContainer& data, std::string schema_name) const {
    std::unique_lock<std::mutex> lock(lookup_mutex_);
    if (!includesSchema(schema_name)) {
        throw schema_not_found_error { "'" + schema_name
                                       + "' is not a registred schema" };
    }
    lock.unlock();

    // we can freely unlock. When a schema has been set it cannot be modified

    if (!validateDataContainer(data, schema_map_.at(schema_name))) {
        // TODO(ploubser): Log valijson eror string when logging at debug level
        // but we've got to wait for leatherman
        throw validation_error { data.toString() + " does not match schema: '" +
                                 schema_name + "'" };
    }
}

bool Validator::includesSchema(std::string schema_name) const {
    return schema_map_.find(schema_name) != schema_map_.end();
}

ContentType Validator::getSchemaContentType(std::string schema_name) const {
    std::unique_lock<std::mutex> lock(lookup_mutex_);
    if (!includesSchema(schema_name)) {
        throw schema_not_found_error { "'" + schema_name +
                                       "' is not a registred schema" };
    }
    lock.unlock();

    return schema_map_.at(schema_name).getContentType();
}

///
/// Private methods
///

bool Validator::validateDataContainer(DataContainer& data,
                                      const Schema& schema) const {
    valijson::Validator validator { schema.getRaw() };
    valijson::adapters::RapidJsonAdapter adapted_document { data.getRaw() };
    valijson::ValidationResults validation_results;

    return validator.validate(adapted_document, &validation_results);
}

}  // namespace CthunClient

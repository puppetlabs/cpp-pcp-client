#include "validator.h"

#include <valijson/adapters/rapidjson_adapter.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validation_results.hpp>
#include <valijson/validator.hpp>

namespace CthunClient {


///
/// Public API
///

Validator::Validator() {
    registerDefaultSchemas();
}

void Validator::registerSchema(const std::string& schema_name, const Schema& schema) {
    std::lock_guard<std::mutex> lock(lookup_mutex_);
    if (includesSchema(schema_name)) {
        throw schema_redefinition_error { "Schema '" + schema_name +
                                          "' already defined." };
    }

    schema_map_[schema_name] = schema;
}

void Validator::validate(DataContainer& data, std::string schema_name) const {
    std::unique_lock<std::mutex> lock (lookup_mutex_);
    if (!includesSchema(schema_name)) {
        throw validation_error { "'" + schema_name + "' is not a registred schema" };
    }
    lock.unlock();

    // we can freely unlock. When a schema has been set it cannot be modified

    if (!validateDataContainer(data, schema_map_.at(schema_name))) {
        // TODO(ploubser): Log valijson eror string when logging at debug level
        // but we've got to wait for leatherman
        throw validation_error { "DataContainer does not match schema: '" +
                                 schema_name + "'" };
    }
}

bool Validator::includesSchema(std::string schema_name) const {
    if (schema_map_.find(schema_name) != schema_map_.end()) {
        return true;
    }

    return false;
}

ContentType Validator::getSchemaContentType(std::string schema_name) const {
    std::unique_lock<std::mutex> lock (lookup_mutex_);
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

void Validator::registerDefaultSchemas() {
    // Envelope Schema
    Schema schema { ContentType::Json };
    schema.addConstraint("id", TypeConstraint::String, true);
    schema.addConstraint("expires", TypeConstraint::String, true);
    schema.addConstraint("sender", TypeConstraint::String, true);
    schema.addConstraint("endpoints", TypeConstraint::Array, true);
    schema.addConstraint("data_schema", TypeConstraint::String, true);
    schema.addConstraint("destination_report", TypeConstraint::Bool, false);
    registerSchema("envelope", schema);
}

bool Validator::validateDataContainer(DataContainer& data, const Schema& schema) const {
    valijson::Validator validator { schema.getRaw() };
    valijson::adapters::RapidJsonAdapter adapted_document { data.getRaw() };
    valijson::ValidationResults validation_results;

    return validator.validate(adapted_document, &validation_results);
}

}  // namespace CthunClient

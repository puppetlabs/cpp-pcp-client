#include <cthun-client/validator/validator.hpp>

#define LEATHERMAN_LOGGING_NAMESPACE CTHUN_CLIENT_LOGGING_PREFIX".validator"
#include <leatherman/logging/logging.hpp>

#include <valijson/adapters/rapidjson_adapter.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validation_results.hpp>
#include <valijson/validator.hpp>

namespace CthunClient {

namespace LTH_JC = leatherman::json_container;

///
/// Auxiliary functions
///

std::string getValidationError(valijson::ValidationResults& validation_results) {
    std::string err_msg {};
    valijson::ValidationResults::Error error;
    unsigned int err_idx { 0 };

    while (validation_results.popError(error)) {
        if (!err_msg.empty()) {
            err_msg += "  - ";
        }
        err_idx++;
        err_msg += "ERROR " + std::to_string(err_idx) + ":";
        for (const auto& context_element : error.context) {
            err_msg += " " + context_element;
        }
    }

    return  err_msg;
}

bool validateJsonContainer(LTH_JC::JsonContainer& data, const Schema& schema) {
    valijson::Validator validator { schema.getRaw() };
    valijson::adapters::RapidJsonAdapter adapted_document { data.getRaw() };
    valijson::ValidationResults validation_results;

    auto success = validator.validate(adapted_document, &validation_results);

    if (!success) {
        auto err_msg = getValidationError(validation_results);
        LOG_DEBUG("Schema validation failure: %1%", err_msg);
    }

    return success;
}

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

void Validator::validate(LTH_JC::JsonContainer& data, std::string schema_name) const {
    std::unique_lock<std::mutex> lock(lookup_mutex_);
    if (!includesSchema(schema_name)) {
        throw schema_not_found_error { "'" + schema_name
                                       + "' is not a registred schema" };
    }
    lock.unlock();

    // we can freely unlock. When a schema has been set it cannot be modified

    if (!validateJsonContainer(data, schema_map_.at(schema_name))) {
        throw validation_error { "does not match schema: '" + schema_name + "'" };
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

}  // namespace CthunClient

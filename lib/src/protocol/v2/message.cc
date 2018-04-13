#include <cpp-pcp-client/protocol/v2/message.hpp>
#include <cpp-pcp-client/protocol/v2/schemas.hpp>

#define LEATHERMAN_LOGGING_NAMESPACE CPP_PCP_CLIENT_LOGGING_PREFIX".message"

#include <leatherman/logging/logging.hpp>

// TODO(ale): disable assert() once we're confident with the code...
// To disable assert()
// #define NDEBUG
#include <cassert>

namespace PCPClient {
namespace v2 {

namespace lth_jc  = leatherman::json_container;

//
// Message
//

// Constructors

Message::Message(lth_jc::JsonContainer envelope) : envelope_(envelope)
{ }

Message::Message(const std::string& transport_msg) :
    Message(lth_jc::JsonContainer(transport_msg))
{ }

static bool validate_data(lth_jc::JsonContainer const& envelope, Validator const& validator)
{
    auto message_type = envelope.get<std::string>("message_type");
    assert(validator.getSchemaContentType(message_type) == ContentType::Json);
    auto data = envelope.get<lth_jc::JsonContainer>("data");

    try {
        validator.validate(data, message_type);
        return true;
    } catch (leatherman::json_container::data_type_error& e) {
        LOG_DEBUG("Invalid data in message {1}: {2}", envelope.get<std::string>("id"), e.what());
    } catch (validator_error& e) {
        LOG_DEBUG("Invalid data in message {1}: {2}", envelope.get<std::string>("id"), e.what());
    }
    return false;
}

ParsedChunks Message::getParsedChunks(const Validator& validator) const {
    validator.validate(envelope_, Protocol::ENVELOPE_SCHEMA_NAME);

    if (envelope_.includes("data")) {
        if (validate_data(envelope_, validator)) {
            return ParsedChunks(envelope_, envelope_.get<lth_jc::JsonContainer>("data"),
                std::vector<lth_jc::JsonContainer>{}, 0);
        } else {
            return ParsedChunks(envelope_, true, std::vector<lth_jc::JsonContainer>{}, 0);
        }
    } else {
        return ParsedChunks(envelope_, std::vector<lth_jc::JsonContainer>{}, 0);
    }
}

// Getter

lth_jc::JsonContainer const& Message::getEnvelope() const {
    return envelope_;
}

// toString

std::string Message::toString() const {
    return envelope_.toString();
}

}  // namespace v2
}  // namespace PCPClient

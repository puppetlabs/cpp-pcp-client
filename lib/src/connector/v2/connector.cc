#include <cpp-pcp-client/connector/v2/connector.hpp>
#include <cpp-pcp-client/protocol/v2/message.hpp>
#include <cpp-pcp-client/protocol/v2/schemas.hpp>
#include <cpp-pcp-client/util/thread.hpp>
#include <cpp-pcp-client/util/chrono.hpp>
#include <cpp-pcp-client/util/logging.hpp>

#define LEATHERMAN_LOGGING_NAMESPACE CPP_PCP_CLIENT_LOGGING_PREFIX".connector"

#include <leatherman/logging/logging.hpp>
#include <leatherman/util/strings.hpp>
#include <leatherman/locale/locale.hpp>

#include <boost/format.hpp>

// TODO(ale): disable assert() once we're confident with the code...
// To disable assert()
// #define NDEBUG
#include <cassert>

namespace PCPClient {
namespace v2 {

namespace lth_jc   = leatherman::json_container;
namespace lth_util = leatherman::util;
namespace lth_loc  = leatherman::locale;

//
// Public api
//

// legacy constructor: pre proxy
Connector::Connector(std::string broker_ws_uri,
                     std::string client_type,
                     std::string ca_crt_path,
                     std::string client_crt_path,
                     std::string client_key_path,
                     long ws_connection_timeout_ms,
                     uint32_t pong_timeouts_before_retry,
                     long ws_pong_timeout_ms)
        : Connector { std::vector<std::string> { std::move(broker_ws_uri) },
                      std::move(client_type),
                      std::move(ca_crt_path),
                      std::move(client_crt_path),
                      std::move(client_key_path),
                      std::move(ws_connection_timeout_ms),
                      std::move(pong_timeouts_before_retry),
                      std::move(ws_pong_timeout_ms)}
{
}

// constructor for proxy addition
Connector::Connector(std::string broker_ws_uri,
                     std::string client_type,
                     std::string ca_crt_path,
                     std::string client_crt_path,
                     std::string client_key_path,
                     std::string ws_proxy,
                     long ws_connection_timeout_ms,
                     uint32_t pong_timeouts_before_retry,
                     long ws_pong_timeout_ms)
        : Connector { std::vector<std::string> { std::move(broker_ws_uri) },
                      std::move(client_type),
                      std::move(ca_crt_path),
                      std::move(client_crt_path),
                      std::move(client_key_path),
                      std::move(ws_proxy),
                      std::move(ws_connection_timeout_ms),
                      std::move(pong_timeouts_before_retry),
                      std::move(ws_pong_timeout_ms)}
{
}

// constructor for crl addition
Connector::Connector(std::string broker_ws_uri,
                     std::string client_type,
                     std::string ca_crt_path,
                     std::string client_crt_path,
                     std::string client_key_path,
                     std::string client_crl_path,
                     std::string ws_proxy,
                     long ws_connection_timeout_ms,
                     uint32_t pong_timeouts_before_retry,
                     long ws_pong_timeout_ms)
        : Connector { std::vector<std::string> { std::move(broker_ws_uri) },
                      std::move(client_type),
                      std::move(ca_crt_path),
                      std::move(client_crt_path),
                      std::move(client_key_path),
                      std::move(client_crl_path),
                      std::move(ws_proxy),
                      std::move(ws_connection_timeout_ms),
                      std::move(pong_timeouts_before_retry),
                      std::move(ws_pong_timeout_ms)}
{
}

// legacy constructor: pre proxy
Connector::Connector(std::vector<std::string> broker_ws_uris,
                     std::string client_type,
                     std::string ca_crt_path,
                     std::string client_crt_path,
                     std::string client_key_path,
                     long ws_connection_timeout_ms,
                     uint32_t pong_timeouts_before_retry,
                     long ws_pong_timeout_ms)
        : ConnectorBase { std::move(broker_ws_uris),
                          std::move(client_type),
                          std::move(ca_crt_path),
                          std::move(client_crt_path),
                          std::move(client_key_path),
                          std::move(ws_connection_timeout_ms),
                          std::move(pong_timeouts_before_retry),
                          std::move(ws_pong_timeout_ms) }
{
    // Rely on ConnectorBase being an abstract class with no operations in the constructor.
    for (auto& broker : broker_ws_uris_) {
        broker += (broker.back() == '/' ? "" : "/") + client_metadata_.client_type;
    }

    // Add PCP schemas to the Validator instance member
    validator_.registerSchema(Protocol::EnvelopeSchema());

    // Register PCP callbacks
    registerMessageCallback(
        Protocol::ErrorMessageSchema(),
        [this](const ParsedChunks& msg) {
            errorMessageCallback(msg);
        });
}

// constructor for proxy addition
Connector::Connector(std::vector<std::string> broker_ws_uris,
                     std::string client_type,
                     std::string ca_crt_path,
                     std::string client_crt_path,
                     std::string client_key_path,
                     std::string ws_proxy,
                     long ws_connection_timeout_ms,
                     uint32_t pong_timeouts_before_retry,
                     long ws_pong_timeout_ms)
        : ConnectorBase { std::move(broker_ws_uris),
                          std::move(client_type),
                          std::move(ca_crt_path),
                          std::move(client_crt_path),
                          std::move(client_key_path),
                          std::move(ws_proxy),
                          std::move(ws_connection_timeout_ms),
                          std::move(pong_timeouts_before_retry),
                          std::move(ws_pong_timeout_ms) }
{
    // Rely on ConnectorBase being an abstract class with no operations in the constructor.
    for (auto& broker : broker_ws_uris_) {
        broker += (broker.back() == '/' ? "" : "/") + client_metadata_.client_type;
    }

    // Add PCP schemas to the Validator instance member
    validator_.registerSchema(Protocol::EnvelopeSchema());

    // Register PCP callbacks
    registerMessageCallback(
        Protocol::ErrorMessageSchema(),
        [this](const ParsedChunks& msg) {
            errorMessageCallback(msg);
        });
}

// constructor for crl addition
Connector::Connector(std::vector<std::string> broker_ws_uris,
                     std::string client_type,
                     std::string ca_crt_path,
                     std::string client_crt_path,
                     std::string client_key_path,
                     std::string client_crl_path,
                     std::string ws_proxy,
                     long ws_connection_timeout_ms,
                     uint32_t pong_timeouts_before_retry,
                     long ws_pong_timeout_ms)
        : ConnectorBase { std::move(broker_ws_uris),
                          std::move(client_type),
                          std::move(ca_crt_path),
                          std::move(client_crt_path),
                          std::move(client_key_path),
                          std::move(client_crl_path),
                          std::move(ws_proxy),
                          std::move(ws_connection_timeout_ms),
                          std::move(pong_timeouts_before_retry),
                          std::move(ws_pong_timeout_ms) }
{
    // Rely on ConnectorBase being an abstract class with no operations in the constructor.
    for (auto& broker : broker_ws_uris_) {
        broker += (broker.back() == '/' ? "" : "/") + client_metadata_.client_type;
    }

    // Add PCP schemas to the Validator instance member
    validator_.registerSchema(Protocol::EnvelopeSchema());

    // Register PCP callbacks
    registerMessageCallback(
        Protocol::ErrorMessageSchema(),
        [this](const ParsedChunks& msg) {
            errorMessageCallback(msg);
        });
}

// Send messages

void Connector::send(const Message& msg)
{
    checkConnectionInitialization();
    auto stringified_msg = msg.toString();
    LOG_DEBUG("Sending message:\n{1}", stringified_msg);
    connection_ptr_->send(stringified_msg);
}

std::string Connector::send(const std::string& target,
                     const std::string& message_type,
                     const lth_jc::JsonContainer& data_json,
                     const std::string& in_reply_to)
{
    auto msg_id = lth_util::get_UUID();
    LOG_DEBUG("Creating message with id {1} for {2} receiver", msg_id, 1);

    lth_jc::JsonContainer envelope_content {};

    envelope_content.set("id", msg_id);
    envelope_content.set("message_type", message_type);
    envelope_content.set("target", target);
    envelope_content.set("sender", client_metadata_.uri);
    envelope_content.set("data", data_json);

    if (!in_reply_to.empty()) {
        envelope_content.set<std::string>("in_reply_to", in_reply_to);
    }

    Message msg { envelope_content };
    send(msg);
    return msg_id;
}

std::string Connector::send(const std::string& target,
                     const std::string& message_type,
                     const std::string& data_txt,
                     const std::string& in_reply_to)
{
    return send(target, message_type, lth_jc::JsonContainer(data_txt), in_reply_to);
}

static lth_jc::JsonContainer asJsonContainer(std::string const& val)
{
    lth_jc::JsonContainer container;
    // set the value as a string ...
    container.set<std::string>("", val);
    // ... but retrieve it as a JSON container
    return container.get<lth_jc::JsonContainer>("");
}

std::string Connector::sendError(const std::string& target,
                     const std::string& in_reply_to,
                     const std::string& description)
{
    lth_jc::JsonContainer error_data = asJsonContainer(description);
    return send(target, Protocol::ERROR_MSG_TYPE, error_data, in_reply_to);
}

//
// Private interface
//

//
// Callbacks
//

// WebSocket - onMessage callback

void Connector::processMessage(const std::string& msg_txt)
{
#ifdef DEV_LOG_RAW_MESSAGE
    LOG_DEBUG("Received message of {1} bytes - raw message:\n{2}",
              msg_txt.size(), msg_txt);
#endif

    std::string err_msg {};

    // Deserialize the incoming message
    std::unique_ptr<Message> msg_ptr;
    try {
        msg_ptr.reset(new Message(msg_txt));
    } catch (const lth_jc::data_parse_error& e) {
        err_msg = lth_loc::format("Invalid envelope - invalid JSON content: {1}",
                                  e.what());
    } catch (const validation_error& e) {
        err_msg = lth_loc::format("Invalid envelope - bad content: {1}", e.what());
    } catch (const schema_not_found_error& e) {
        // This is unexpected
        err_msg = lth_loc::format("Unknown schema: {1}", e.what());
    }

    if (!err_msg.empty()) {
        // Log and return; we cannot break the WebSocket event loop
        LOG_ERROR(err_msg);
        LOG_ACCESS((boost::format("DESERIALIZATION_ERROR %1% unknown unknown unknown")
                    % connection_ptr_->getWsUri()).str());
        return;
    }

    auto chunks = msg_ptr->getParsedChunks(validator_);
    lth_jc::JsonContainer const& envelope = chunks.envelope;

    auto message_type = envelope.get<std::string>("message_type");
    auto id = envelope.get<std::string>("id");
    auto sender = envelope.includes("sender") ? envelope.get<std::string>("sender") : MY_BROKER_URI;
    LOG_ACCESS((boost::format("AUTHORIZATION_SUCCESS %1% %2% %3% %4%")
                   % connection_ptr_->getWsUri() % sender % message_type % id).str());

    // Execute the callback associated with the data schema
    if (schema_callback_pairs_.find(message_type) != schema_callback_pairs_.end()) {
        auto c_b = schema_callback_pairs_.at(message_type);
        LOG_TRACE("Executing callback for a message with '{1}' schema", message_type);
        c_b(chunks);
    } else {
        LOG_WARNING("No message callback has been registered for the '{1}' schema",
                    message_type);
    }
}

// PCP - PCP Error message callback

void Connector::errorMessageCallback(const ParsedChunks& chunks)
{
    lth_jc::JsonContainer envelope = chunks.envelope;

    auto error_id = envelope.get<std::string>("id");
    auto sender_uri = envelope.includes("sender") ? envelope.get<std::string>("sender") : MY_BROKER_URI;

    std::string description;
    if (chunks.has_data && !chunks.invalid_data) {
        assert(chunks.data_type == ContentType::Json);
        description = chunks.data.get<std::string>();
    }

    std::string cause_id {};
    std::string error_msg { lth_loc::format("Received error {1} from {2}", error_id, sender_uri) };

    if (envelope.includes("in_reply_to")) {
        cause_id = envelope.get<std::string>("in_reply_to");
        LOG_WARNING("{1} caused by message {2}: {3}", error_msg, cause_id, description);
    } else {
        LOG_WARNING("{1} (the id of the message that caused it is unknown): {2}",
                    error_msg, description);
    }

    if (error_callback_)
        error_callback_(chunks);
}

}  // namespace v1
}  // namespace PCPClient

#include <cpp-pcp-client/connector/connector.hpp>
#include <cpp-pcp-client/protocol/message.hpp>
#include <cpp-pcp-client/protocol/schemas.hpp>
#include <cpp-pcp-client/util/thread.hpp>
#include <cpp-pcp-client/util/chrono.hpp>

#define LEATHERMAN_LOGGING_NAMESPACE CPP_PCP_CLIENT_LOGGING_PREFIX".connector"

#include <leatherman/logging/logging.hpp>

#include <leatherman/util/strings.hpp>
#include <leatherman/util/time.hpp>

#include <cstdio>

// TODO(ale): disable assert() once we're confident with the code...
// To disable assert()
// #define NDEBUG
#include <cassert>

namespace PCPClient {

namespace lth_jc = leatherman::json_container;
namespace lth_util = leatherman::util;

//
// Constants
//

static const uint32_t CONNECTION_CHECK_S { 15 };  // [s]
static const int DEFAULT_MSG_TIMEOUT { 10 };  // [s]

static const std::string MY_BROKER_URI { "pcp:///server" };

//
// Public api
//

Connector::Connector(const std::string& broker_ws_uri,
                     const std::string& client_type,
                     const std::string& ca_crt_path,
                     const std::string& client_crt_path,
                     const std::string& client_key_path,
                     const long& connection_timeout)
        : broker_ws_uri_ { broker_ws_uri },
          client_metadata_ { client_type,
                             ca_crt_path,
                             client_crt_path,
                             client_key_path,
                             connection_timeout },
          connection_ptr_ { nullptr },
          validator_ {},
          schema_callback_pairs_ {},
          error_callback_ {},
          associate_response_callback_ {},
          mutex_ {},
          cond_var_ {},
          is_destructing_ { false },
          is_monitoring_ { false },
          is_associated_ { false } {
    // Add PCP schemas to the Validator instance member
    validator_.registerSchema(Protocol::EnvelopeSchema());
    validator_.registerSchema(Protocol::DebugSchema());
    validator_.registerSchema(Protocol::DebugItemSchema());

    // Register PCP callbacks
    registerMessageCallback(
        Protocol::AssociateResponseSchema(),
        [this](const ParsedChunks& parsed_chunks) {
            associateResponseCallback(parsed_chunks);
        });

    registerMessageCallback(
        Protocol::ErrorMessageSchema(),
        [this](const ParsedChunks& parsed_chunks) {
            errorMessageCallback(parsed_chunks);
        });
}

Connector::~Connector() {
    if (connection_ptr_ != nullptr) {
        // reset callbacks to avoid breaking the Connection instance
        // due to callbacks having an invalid reference context
        LOG_INFO("Resetting the WebSocket event callbacks");
        connection_ptr_->resetCallbacks();
    }

    {
        Util::lock_guard<Util::mutex> the_lock { mutex_ };
        is_destructing_ = true;
        cond_var_.notify_one();
    }
}

// Register schemas and onMessage callbacks
void Connector::registerMessageCallback(const Schema schema,
                                        MessageCallback callback) {
    validator_.registerSchema(schema);
    auto p = std::pair<std::string, MessageCallback>(schema.getName(), callback);
    schema_callback_pairs_.insert(p);
}

// Set an optional callback for error messages
void Connector::setPCPErrorCallback(MessageCallback callback) {
    error_callback_ = callback;
}

// Set an optional callback for associate responses
void Connector::setAssociateCallback(MessageCallback callback) {
    associate_response_callback_ = callback;
}


// Manage the connection state

void Connector::connect(int max_connect_attempts) {
    if (connection_ptr_ == nullptr) {
        // Initialize the WebSocket connection
        connection_ptr_.reset(new Connection(broker_ws_uri_, client_metadata_));

        // Set WebSocket callbacks
        connection_ptr_->setOnMessageCallback(
            [this](std::string message) {
                processMessage(message);
            });

        connection_ptr_->setOnOpenCallback(
            [this]() {
                associateSession();
            });
    }

    try {
        // Open the WebSocket connection
        connection_ptr_->connect(max_connect_attempts);
    } catch (const connection_processing_error& e) {
        // NB: connection_fatal_errors (can't connect after n tries)
        //     and _config_errors (TLS initialization error) are
        //     propagated whereas _processing_errors are converted
        //     to _config_errors (they can be thrown by the
        //     synchronous call websocketpp::Endpoint::connect())
        LOG_ERROR("Failed to connect: %1%", e.what());
        throw connection_config_error { e.what() };
    }
}

bool Connector::isConnected() const {
    return connection_ptr_ != nullptr
           && connection_ptr_->getConnectionState() == ConnectionStateValues::open;
}

bool Connector::isAssociated() const {
    return isConnected() && is_associated_.load();
}

void Connector::monitorConnection(int max_connect_attempts) {
    checkConnectionInitialization();

    if (!is_monitoring_) {
        is_monitoring_ = true;
        startMonitorTask(max_connect_attempts);
    } else {
        LOG_WARNING("The monitorConnection has already been called");
    }
}

// Send messages

void Connector::send(const Message& msg) {
    checkConnectionInitialization();
    auto serialized_msg = msg.getSerialized();
    LOG_DEBUG("Sending message of %1% bytes:\n%2%",
              serialized_msg.size(), msg.toString());
    connection_ptr_->send(&serialized_msg[0], serialized_msg.size());
}

void Connector::send(const std::vector<std::string>& targets,
                     const std::string& message_type,
                     unsigned int timeout,
                     const lth_jc::JsonContainer& data_json,
                     const std::vector<lth_jc::JsonContainer>& debug) {
    sendMessage(targets,
                message_type,
                timeout,
                false,
                data_json.toString(),
                debug);
}

void Connector::send(const std::vector<std::string>& targets,
                     const std::string& message_type,
                     unsigned int timeout,
                     const std::string& data_binary,
                     const std::vector<lth_jc::JsonContainer>& debug) {
    sendMessage(targets,
                message_type,
                timeout,
                false,
                data_binary,
                debug);
}

void Connector::send(const std::vector<std::string>& targets,
                     const std::string& message_type,
                     unsigned int timeout,
                     bool destination_report,
                     const lth_jc::JsonContainer& data_json,
                     const std::vector<lth_jc::JsonContainer>& debug) {
    sendMessage(targets,
                message_type,
                timeout,
                destination_report,
                data_json.toString(),
                debug);
}

void Connector::send(const std::vector<std::string>& targets,
                     const std::string& message_type,
                     unsigned int timeout,
                     bool destination_report,
                     const std::string& data_binary,
                     const std::vector<lth_jc::JsonContainer>& debug) {
    sendMessage(targets,
                message_type,
                timeout,
                destination_report,
                data_binary,
                debug);
}

//
// Private interface
//

// Utility functions

void Connector::checkConnectionInitialization() {
    if (connection_ptr_ == nullptr) {
        throw connection_not_init_error { "connection not initialized" };
    }
}

MessageChunk Connector::createEnvelope(const std::vector<std::string>& targets,
                                       const std::string& message_type,
                                       unsigned int timeout,
                                       bool destination_report) {
    auto msg_id = lth_util::get_UUID();
    auto expires = lth_util::get_ISO8601_time(timeout);
    LOG_DEBUG("Creating message with id %1% for %2% receiver%3%",
              msg_id, targets.size(), lth_util::plural(targets.size()));

    lth_jc::JsonContainer envelope_content {};

    envelope_content.set<std::string>("id", msg_id);
    envelope_content.set<std::string>("message_type", message_type);
    envelope_content.set<std::vector<std::string>>("targets", targets);
    envelope_content.set<std::string>("expires", expires);
    envelope_content.set<std::string>("sender", client_metadata_.uri);

    if (destination_report) {
        envelope_content.set<bool>("destination_report", true);
    }

    return MessageChunk { ChunkDescriptor::ENVELOPE, envelope_content.toString() };
}

void Connector::sendMessage(const std::vector<std::string>& targets,
                            const std::string& message_type,
                            unsigned int timeout,
                            bool destination_report,
                            const std::string& data_txt,
                            const std::vector<lth_jc::JsonContainer>& debug) {
    auto envelope_chunk = createEnvelope(targets, message_type, timeout,
                                         destination_report);
    MessageChunk data_chunk { ChunkDescriptor::DATA, data_txt };
    Message msg { envelope_chunk, data_chunk };

    for (auto debug_content : debug) {
        MessageChunk d_c { ChunkDescriptor::DEBUG, debug_content.toString() };
        msg.addDebugChunk(d_c);
    }

    send(msg);
}

// WebSocket onOpen callback - will send the associate session request

void Connector::associateSession() {
    // Envelope
    auto envelope = createEnvelope(std::vector<std::string> { MY_BROKER_URI },
                                   Protocol::ASSOCIATE_REQ_TYPE,
                                   DEFAULT_MSG_TIMEOUT,
                                   false);

    // Create and send message
    Message msg { envelope };
    LOG_INFO("Sending Associate Session request");
    send(msg);
}

// WebSocket onMessage callback

void Connector::processMessage(const std::string& msg_txt) {
#ifdef DEV_LOG_RAW_MESSAGE
    LOG_DEBUG("Received message of %1% bytes - raw message:\n%2%",
              msg_txt.size(), msg_txt);
#endif

    // Deserialize the incoming message
    std::unique_ptr<Message> msg_ptr;
    try {
        msg_ptr.reset(new Message(msg_txt));
    } catch (message_error& e) {
        LOG_ERROR("Failed to deserialize message: %1%", e.what());
        return;
    }

    // Parse message chunks
    ParsedChunks parsed_chunks;
    try {
        parsed_chunks = msg_ptr->getParsedChunks(validator_);
    } catch (validation_error& e) {
        LOG_ERROR("Invalid envelope - bad content: %1%", e.what());
        return;
    } catch (lth_jc::data_parse_error& e) {
        LOG_ERROR("Invalid envelope - invalid JSON content: %1%", e.what());
        return;
    } catch (schema_not_found_error& e) {
        // This is unexpected
        LOG_ERROR("Unknown schema: %1%", e.what());
        return;
    }

    // Execute the callback associated with the data schema
    auto schema_name = parsed_chunks.envelope.get<std::string>("message_type");

    if (schema_callback_pairs_.find(schema_name) != schema_callback_pairs_.end()) {
        auto c_b = schema_callback_pairs_.at(schema_name);
        LOG_TRACE("Executing callback for a message with '%1%' schema",
                  schema_name);
        c_b(parsed_chunks);
    } else {
        LOG_WARNING("No message callback has be registered for '%1%' schema",
                    schema_name);
    }
}

// Associate session response callback

void Connector::associateResponseCallback(const ParsedChunks& parsed_chunks) {
    assert(parsed_chunks.has_data);
    assert(parsed_chunks.data_type == PCPClient::ContentType::Json);

    auto response_id = parsed_chunks.envelope.get<std::string>("id");
    auto sender_uri = parsed_chunks.envelope.get<std::string>("sender");

    auto request_id = parsed_chunks.data.get<std::string>("id");
    auto success = parsed_chunks.data.get<bool>("success");

    std::string msg { "Received associate session response " + response_id
                      + " from " + sender_uri + " for request " + request_id };

    if (success) {
        LOG_INFO("%1%: success", msg);
        is_associated_ = true;
    } else {
        if (parsed_chunks.data.includes("reason")) {
            auto reason = parsed_chunks.data.get<std::string>("reason");
            LOG_WARNING("%1%: failure - %2%", msg, reason);
        } else {
            LOG_WARNING("%1%: failure", msg);
        }
    }

    if (associate_response_callback_) {
        associate_response_callback_(parsed_chunks);
    }
}

// PCP error message callback

void Connector::errorMessageCallback(const ParsedChunks& parsed_chunks) {
    assert(parsed_chunks.has_data);
    assert(parsed_chunks.data_type == PCPClient::ContentType::Json);

    auto error_id = parsed_chunks.envelope.get<std::string>("id");
    auto sender_uri = parsed_chunks.envelope.get<std::string>("sender");

    auto description = parsed_chunks.data.get<std::string>("description");

    std::string msg { "Received error " + error_id + " from " + sender_uri };

    if (parsed_chunks.data.includes("id")) {
        auto cause_id = parsed_chunks.data.get<std::string>("id");
        LOG_WARNING("%1% caused by message %2%: %3%", msg, cause_id, description);
    } else {
        LOG_WARNING("%1% (the id of the message that caused it is unknown): %2%",
                    msg, description);
    }

    if (error_callback_) {
        error_callback_(parsed_chunks);
    }
}

// Monitor task

void Connector::startMonitorTask(int max_connect_attempts) {
    assert(connection_ptr_ != nullptr);

    while (true) {
        Util::unique_lock<Util::mutex> the_lock { mutex_ };
        auto now = Util::chrono::system_clock::now();

        cond_var_.wait_until(the_lock,
                             now + Util::chrono::seconds(CONNECTION_CHECK_S));

        if (is_destructing_) {
            // The dtor has been invoked
            LOG_INFO("Stopping the monitor task");
            is_monitoring_ = false;
            the_lock.unlock();
            return;
        }

        try {
            if (!isConnected()) {
                LOG_WARNING("WebSocket connection to PCP broker lost; retrying");
                is_associated_ = false;
                connection_ptr_->connect(max_connect_attempts);
            } else {
                LOG_DEBUG("Sending heartbeat ping");
                connection_ptr_->ping();
            }
        } catch (const connection_processing_error& e) {
            // Connection::connect() or ping() failure - keep trying
            LOG_WARNING("The connection monitor task will continue after a "
                        "WebSocket failure (%1%)", e.what());
        } catch (const connection_config_error& e) {
            // WebSocket TLS init error - keep trying
            LOG_WARNING("The connection monitor task will continue after a "
                        "WebSocket configuration failure (%1%)", e.what());
        } catch (const connection_fatal_error& e) {
            // Failed to reconnect after max_connect_attempts - stop
            LOG_WARNING("The connection monitor task will stop: %1%",
                        e.what());
            is_monitoring_ = false;
            the_lock.unlock();
            throw;
        }

        the_lock.unlock();
    }
}

}  // namespace PCPClient

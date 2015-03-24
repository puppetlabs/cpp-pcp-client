#include <cthun-client/connector/connector.h>
#include <cthun-client/connector/errors.h>
#include <cthun-client/connector/uuid.h>
#include <cthun-client/message/message.h>

#define LEATHERMAN_LOGGING_NAMESPACE CTHUN_CLIENT_LOGGING_PREFIX".connector"

#include <leatherman/logging/logging.hpp>

#include <boost/date_time/posix_time/posix_time.hpp>

#include <cstdio>
#include <chrono>
namespace CthunClient {

//
// Constants
//

static const uint CONNECTION_CHECK_S { 15 };  // [s]
static const int DEFAULT_MSG_TIMEOUT { 10 };  // [s]

//
// Utility functions
//

// TODO(ale): move this to leatherman
std::string getISO8601Time(unsigned int modifier_in_seconds) {
    boost::posix_time::ptime t = boost::posix_time::microsec_clock::universal_time()
                                 + boost::posix_time::seconds(modifier_in_seconds);
    return boost::posix_time::to_iso_extended_string(t) + "Z";
}

// TODO(ale): move plural from the common StringUtils in leatherman
template<typename T>
std::string plural(std::vector<T> things);

std::string plural(int num_of_things) {
    return num_of_things > 1 ? "s" : "";
}

//
// Public api
//

Connector::Connector(const std::string& server_url,
                     const std::string& type,
                     const std::string& ca_crt_path,
                     const std::string& client_crt_path,
                     const std::string& client_key_path)
        : server_url_ { server_url },
          client_metadata_ { type, ca_crt_path, client_crt_path, client_key_path },
          connection_ptr_ { nullptr },
          validator_ {},
          schema_callback_pairs_ {},
          mutex_ {},
          cond_var_ {},
          is_destructing_ { false },
          is_monitoring_ { false } {
    addEnvelopeSchemaToValidator();
}

Connector::~Connector() {
    if (connection_ptr_ != nullptr) {
        // reset callbacks to avoid breaking the Connection instance
        // due to callbacks having an invalid reference context
        LOG_INFO("Resetting the WebSocket event callbacks");
        connection_ptr_->resetCallbacks();
    }

    {
        std::lock_guard<std::mutex> the_lock { mutex_ };
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

// Manage the connection state

void Connector::connect(int max_connect_attempts) {
    if (connection_ptr_ == nullptr) {
        // Initialize the WebSocket connection
        connection_ptr_.reset(new Connection(server_url_, client_metadata_));

        connection_ptr_->setOnMessageCallback(
            [this](std::string message) {
                processMessage(message);
            });

        connection_ptr_->setOnOpenCallback(
            [this]() {
                sendLogin();
            });
    }

    try {
        // Open the WebSocket connection
        connection_ptr_->connect(max_connect_attempts);
    } catch (connection_processing_error& e) {
        // NB: connection_fatal_errors are propagated whereas
        //     connection_processing_errors are converted to
        //     connection_config_errors (they can be thrown after
        //     websocketpp::Endpoint::connect() or ::send() failures)
        LOG_ERROR("Failed to connect: %1%", e.what());
        throw connection_config_error { e.what() };
    }
}

bool Connector::isConnected() const {
    // TODO(ale): make this consistent with the login transaction as
    // specified in the protocol specs (perhaps with a logged-in flag)

    return connection_ptr_ != nullptr
           && connection_ptr_->getConnectionState() == ConnectionStateValues::open;
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

void Connector::send(const std::vector<std::string>& endpoints,
                     const std::string& data_schema,
                     unsigned int timeout,
                     const DataContainer& data_json,
                     const std::vector<DataContainer>& debug) {
    sendMessage(endpoints,
                data_schema,
                timeout,
                data_json.toString(),
                debug);
}

void Connector::send(const std::vector<std::string>& endpoints,
                     const std::string& data_schema,
                     unsigned int timeout,
                     const std::string& data_binary,
                     const std::vector<DataContainer>& debug) {
    sendMessage(endpoints,
                data_schema,
                timeout,
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

void Connector::addEnvelopeSchemaToValidator() {
    Schema schema { ENVELOPE_SCHEMA_NAME, ContentType::Json };

    schema.addConstraint("id", TypeConstraint::String, true);
    schema.addConstraint("expires", TypeConstraint::String, true);
    schema.addConstraint("sender", TypeConstraint::String, true);
    schema.addConstraint("endpoints", TypeConstraint::Array, true);
    schema.addConstraint("data_schema", TypeConstraint::String, true);
    schema.addConstraint("destination_report", TypeConstraint::Bool, false);

    validator_.registerSchema(schema);
}

MessageChunk Connector::createEnvelope(const std::vector<std::string>& endpoints,
                                       const std::string& data_schema,
                                       unsigned int timeout) {
    auto msg_id = UUID::getUUID();
    auto expires = getISO8601Time(timeout);
    LOG_INFO("Creating message with id %1% for %2% receiver%3%",
             msg_id, endpoints.size(), plural(endpoints.size()));

    DataContainer envelope_content {};

    envelope_content.set<std::string>("id", msg_id);
    envelope_content.set<std::string>("expires", expires);
    envelope_content.set<std::string>("sender", client_metadata_.id);
    envelope_content.set<std::string>("data_schema", data_schema);
    envelope_content.set<std::vector<std::string>>("endpoints", endpoints);

    return MessageChunk { ChunkDescriptor::ENVELOPE, envelope_content.toString() };
}

void Connector::sendMessage(const std::vector<std::string>& endpoints,
                            const std::string& data_schema,
                            unsigned int timeout,
                            const std::string& data_txt,
                            const std::vector<DataContainer>& debug) {
    auto envelope_chunk = createEnvelope(endpoints, data_schema, timeout);
    MessageChunk data_chunk { ChunkDescriptor::DATA, data_txt };
    Message msg { envelope_chunk, data_chunk };

    for (auto debug_content : debug) {
        MessageChunk d_c { ChunkDescriptor::DEBUG, debug_content.toString() };
        msg.addDebugChunk(d_c);
    }

    send(msg);
}

// Login

void Connector::sendLogin() {
    // Envelope
    auto envelope = createEnvelope(std::vector<std::string> { "cth://server" },
                                   CTHUN_LOGIN_SCHEMA_NAME,
                                   DEFAULT_MSG_TIMEOUT);

    // Data
    DataContainer data_entries {};
    data_entries.set<std::string>("type", client_metadata_.type);
    MessageChunk data { ChunkDescriptor::DATA, data_entries.toString() };

    // Create and send message
    Message msg { envelope, data };
    LOG_INFO("Sending login message");
    LOG_DEBUG("Login message data: %1%", data.content);
    send(msg);
}

// WebSocket onMessage callback

void Connector::processMessage(const std::string& msg_txt) {
    LOG_DEBUG("Received message of %1% bytes:\n%2%", msg_txt.size(), msg_txt);

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
    } catch (validator_error& e) {
        LOG_ERROR("Invalid message: %1%", e.what());
        return;
    }

    // Execute the callback associated with the data schema
    auto schema_name = parsed_chunks.envelope.get<std::string>("data_schema");

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

// Monitor task

void Connector::startMonitorTask(int max_connect_attempts) {
    assert(connection_ptr_ != nullptr);

    while (true) {
        std::unique_lock<std::mutex> the_lock { mutex_ };
        auto now = std::chrono::system_clock::now();

        cond_var_.wait_until(the_lock,
                             now + std::chrono::seconds(CONNECTION_CHECK_S));

        if (is_destructing_) {
            // The dtor has been invoked
            LOG_INFO("Stopping the monitor task");
            is_monitoring_ = false;
            the_lock.unlock();
            return;
        }

        try {
            if (!isConnected()) {
                LOG_WARNING("Connection to Cthun server lost; retrying");
                connection_ptr_->connect(max_connect_attempts);
            } else {
                LOG_DEBUG("Sending heartbeat ping");
                connection_ptr_->ping();
            }
        } catch (connection_processing_error& e) {
            // Connection::connect() or ping() failure - keep trying
            LOG_ERROR("Connection monitor failure: %1%", e.what());
        } catch (connection_fatal_error& e) {
            // Failed to reconnect after max_connect_attempts - stop
            LOG_ERROR("The connection monitor task will stop - failure: %1%",
                      e.what());
            is_monitoring_ = false;
            the_lock.unlock();
            throw;
        }

        the_lock.unlock();
    }
}

}  // namespace CthunClient

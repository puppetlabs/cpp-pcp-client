#include "./connector.h"
#include "./errors.h"
#include "./uuid.h"

#include "../message/errors.h"

// TODO(ale): include logging library and uncomment logging macros

#include <boost/date_time/posix_time/posix_time.hpp>

#include <cstdio>
#include <iostream>

namespace CthunClient {

//
// Constants
//

static const uint CONNECTION_CHECK_INTERVAL { 15 };  // [s]
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
          monitor_task_ {},
          mutex_ {},
          cond_var_ {},
          is_destructing_ { false },
          is_monitoring_ { false },
          monitor_timer_ {} {
    addEnvelopeSchemaToValidator_();
}

Connector::~Connector() {
    {
        std::lock_guard<std::mutex> the_lock { mutex_ };
        is_destructing_ = true;
        cond_var_.notify_one();
    }

    if (monitor_task_.joinable()) {
        monitor_task_.join();
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
                processMessage_(message);
            });
    }

    try {
        // Open the WebSocket connection
        connection_ptr_->connect(max_connect_attempts);
        // Send the login message
        sendLogin_();
    } catch (connection_error& e) {
        // LOG_ERROR("Failed to connect: %1%", e.what());
        throw connection_fatal_error { "failed to connect" };
    }
}

bool Connector::isConnected() const {
    // TODO(ale): make this consistent with the login transaction as
    // specified in the protocol specs (perhaps with a logged-in flag)

    return connection_ptr_ != nullptr
           && connection_ptr_->getConnectionState() == ConnectionStateValues::open;
}

void Connector::enablePersistence(int max_connect_attempts) {
    checkConnectionInitialization_();

    if (!is_monitoring_) {
        is_monitoring_ = true;
        monitor_task_ = std::move(
            std::thread(&Connector::monitorConnectionTask_,
                        this, max_connect_attempts));
    } else {
        // LOG_WARNING("The monitoring task that enables persistence "
        //             "has already started")
    }
}

// Send messages

void Connector::send(const Message& msg) {
    checkConnectionInitialization_();
    auto serialized_msg = msg.getSerialized();
    connection_ptr_->send(&serialized_msg[0], serialized_msg.size());
}

void Connector::send(std::vector<std::string> endpoints,
                     std::string data_schema,
                     unsigned int timeout,
                     DataContainer data_json,
                     std::vector<DataContainer> debug) {
    sendMessage_(endpoints,
                 data_schema,
                 timeout,
                 data_json.toString(),
                 debug);
}

void Connector::send(std::vector<std::string> endpoints,
                     std::string data_schema,
                     unsigned int timeout,
                     std::string data_binary,
                     std::vector<DataContainer> debug) {
    sendMessage_(endpoints,
                 data_schema,
                 timeout,
                 data_binary,
                 debug);
}

//
// Private interface
//

// Utility functions

void Connector::checkConnectionInitialization_() {
    if (connection_ptr_ == nullptr) {
        throw connection_not_init_error { "connection not initialized" };
    }
}

void Connector::addEnvelopeSchemaToValidator_() {
    Schema schema { ENVELOPE_SCHEMA_NAME, ContentType::Json };

    schema.addConstraint("id", TypeConstraint::String, true);
    schema.addConstraint("expires", TypeConstraint::String, true);
    schema.addConstraint("sender", TypeConstraint::String, true);
    schema.addConstraint("endpoints", TypeConstraint::Array, true);
    schema.addConstraint("data_schema", TypeConstraint::String, true);
    schema.addConstraint("destination_report", TypeConstraint::Bool, false);

    validator_.registerSchema(schema);
}

MessageChunk Connector::createEnvelope_(const std::vector<std::string>& endpoints,
                                        const std::string& data_schema,
                                        unsigned int timeout) {
    auto msg_id = UUID::getUUID();
    auto expires = getISO8601Time(timeout);
    DataContainer envelope_content {};

    envelope_content.set<std::string>("id", msg_id);
    envelope_content.set<std::string>("expires", expires);
    envelope_content.set<std::string>("sender", client_metadata_.id);
    envelope_content.set<std::string>("data_schema", data_schema);
    envelope_content.set<std::vector<std::string>>("endpoints", endpoints);

    return MessageChunk { ChunkDescriptor::ENVELOPE, envelope_content.toString() };
}

void Connector::sendMessage_(std::vector<std::string> endpoints,
                             std::string data_schema,
                             unsigned int timeout,
                             std::string data_txt,
                             std::vector<DataContainer> debug) {
    auto envelope_chunk = createEnvelope_(endpoints, data_schema, timeout);
    MessageChunk data_chunk { ChunkDescriptor::DATA, data_txt };
    Message msg { envelope_chunk, data_chunk };

    for (auto debug_content: debug) {
        MessageChunk d_c { ChunkDescriptor::DEBUG, debug_content.toString() };
        msg.addDebugChunk(d_c);
    }

    send(msg);
}

// Login

void Connector::sendLogin_() {
    // Envelope
    auto envelope = createEnvelope_(std::vector<std::string> { "cth://server" },
                                    CTHUN_LOGIN_SCHEMA_NAME,
                                    DEFAULT_MSG_TIMEOUT);

    // Data
    DataContainer data_entries {};
    data_entries.set<std::string>("type", client_metadata_.type);
    MessageChunk data { ChunkDescriptor::DATA, data_entries.toString() };

    // Create and send message
    Message msg { envelope, data };
    // LOG_INFO("Sending login message with id: %1%",
    //          envelope_entries.get<std::string>("id"));
    // LOG_DEBUG("Login message data: %1%", data.content);
    send(msg);
}

// WebSocket onMessage callback

void Connector::processMessage_(std::string msg_txt) {
    // LOG_DEBUG("Received message:\n%1%", msg_txt);

    // Deserialize the incoming message
    std::unique_ptr<Message> msg_ptr;
    try {
        msg_ptr.reset(new Message(msg_txt));
    } catch (message_error& e) {
        // LOG_ERROR("Failed to deserialize message: %1%", e.what());
        return;
    }

    // Parse message chunks
    ParsedChunks parsed_chunks;
    try {
        parsed_chunks = msg_ptr->getParsedChunks(validator_);
    } catch (validator_error& e) {
        // LOG_ERROR("Invalid message: %1%", e.what());
        return;
    }

    // Execute the callback associated with the data schema
    auto schema_name = parsed_chunks.envelope.get<std::string>("data_schema");

    if (schema_callback_pairs_.find(schema_name) != schema_callback_pairs_.end()) {
        auto c_b = schema_callback_pairs_.at(schema_name);
        // LOG_TRACE("Executing callback for a message with '%1%' schema",
        //           schema_name);
        c_b(parsed_chunks);
    } else {
        // LOG_WARNING("No message callback has be registered for '%1%' schema",
        //             schema_name);
    }
}

// Monitor task

void Connector::monitorConnectionTask_(int max_connect_attempts) {
    assert(connection_ptr_ != nullptr);

    while(true) {
        std::unique_lock<std::mutex> the_lock { mutex_ };
        monitor_timer_.reset();

        cond_var_.wait(the_lock,
            [this] {
                return is_destructing_
                       || monitor_timer_.elapsedSeconds() < CONNECTION_CHECK_INTERVAL;
            }
        );

        if (is_destructing_) {
            // The dtor has been invoked
            // LOG_INFO("Stopping the monitor task (persistence)");
            return;
        }

        try {
            if (connection_ptr_->getConnectionState() != ConnectionStateValues::open) {
                // LOG_WARNING("Connection to Cthun server lost; retrying");
                connection_ptr_->connect(max_connect_attempts);
            } else {
                // LOG_DEBUG("Sending heartbeat ping");
                connection_ptr_->ping();
            }
        } catch (connection_error& e) {
            // LOG_ERROR("Monitoring task (persistence) failure: %1%", e.what());
        }

        the_lock.unlock();
    }
}

}  // namespace CthunClient

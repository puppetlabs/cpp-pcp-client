#include <cpp-pcp-client/connector/connector.hpp>
#include <cpp-pcp-client/protocol/message.hpp>
#include <cpp-pcp-client/protocol/schemas.hpp>
#include <leatherman/util/thread.hpp>
#include <leatherman/util/chrono.hpp>

#define LEATHERMAN_LOGGING_NAMESPACE CPP_PCP_CLIENT_LOGGING_PREFIX".connector"

#include <leatherman/logging/logging.hpp>

#include <leatherman/util/strings.hpp>
#include <leatherman/util/time.hpp>
#include <leatherman/util/timer.hpp>

#include <leatherman/locale/locale.hpp>

#include <cstdio>

// TODO(ale): disable assert() once we're confident with the code...
// To disable assert()
// #define NDEBUG
#include <cassert>

namespace PCPClient {

namespace lth_jc   = leatherman::json_container;
namespace lth_util = leatherman::util;
namespace lth_loc  = leatherman::locale;

//
// Constants
//

static const int WS_CONNECTION_CLOSE_TIMEOUT_S { 5 };  // [s]
static const std::string MY_BROKER_URI { "pcp:///server" };

//
// Public api
//

Connector::Connector(std::string broker_ws_uri,
                     std::string client_type,
                     std::string ca_crt_path,
                     std::string client_crt_path,
                     std::string client_key_path,
                     long ws_connection_timeout_ms,
                     uint32_t association_timeout_s,
                     uint32_t association_request_ttl_s,
                     uint32_t pong_timeouts_before_retry,
                     long ws_pong_timeout_ms)
        : Connector { std::vector<std::string> { std::move(broker_ws_uri) },
                      std::move(client_type),
                      std::move(ca_crt_path),
                      std::move(client_crt_path),
                      std::move(client_key_path),
                      std::move(ws_connection_timeout_ms),
                      std::move(association_timeout_s),
                      std::move(association_request_ttl_s),
                      std::move(pong_timeouts_before_retry),
                      std::move(ws_pong_timeout_ms)}
{
}

Connector::Connector(std::vector<std::string> broker_ws_uris,
                     std::string client_type,
                     std::string ca_crt_path,
                     std::string client_crt_path,
                     std::string client_key_path,
                     long ws_connection_timeout_ms,
                     uint32_t association_timeout_s,
                     uint32_t association_request_ttl_s,
                     uint32_t pong_timeouts_before_retry,
                     long ws_pong_timeout_ms)
        : connection_ptr_ { nullptr },
          broker_ws_uris_ { std::move(broker_ws_uris) },
          client_metadata_ { std::move(client_type),
                             std::move(ca_crt_path),
                             std::move(client_crt_path),
                             std::move(client_key_path),
                             std::move(ws_connection_timeout_ms),
                             std::move(association_timeout_s),
                             std::move(association_request_ttl_s),
                             std::move(pong_timeouts_before_retry),
                             std::move(ws_pong_timeout_ms)},
          validator_ {},
          schema_callback_pairs_ {},
          error_callback_ {},
          associate_response_callback_ {},
          is_monitoring_ { false },
          monitor_thread_ {},
          monitor_mutex_ {},
          monitor_cond_var_ {},
          must_stop_monitoring_ { false },
          session_association_ {}
{
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

    registerMessageCallback(
        Protocol::TTLExpiredSchema(),
        [this](const ParsedChunks& parsed_chunks) {
            TTLMessageCallback(parsed_chunks);
        });
}

Connector::~Connector()
{
    if (connection_ptr_ != nullptr) {
        // reset callbacks to avoid breaking the Connection instance
        // due to callbacks having an invalid reference context
        LOG_INFO("Resetting the WebSocket event callbacks");
        connection_ptr_->resetCallbacks();
    }

    try {
        if (is_monitoring_) {
            stopMonitorTaskAndWait();
        } else if (monitor_exception_) {
            std::rethrow_exception(monitor_exception_);
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Error previously caught by the Monitor Thread: {1}", e.what());
    } catch (...) {
        LOG_ERROR("An unexpected error has been previously caught by the Monitor Thread");
    }
}

// Register schemas and onMessage callbacks
void Connector::registerMessageCallback(const lth_jc::Schema& schema,
                                        MessageCallback callback)
{
    validator_.registerSchema(schema);
    auto p = std::pair<std::string, MessageCallback>(schema.getName(), callback);
    schema_callback_pairs_.insert(p);
}

// Set an optional callback for error messages
void Connector::setPCPErrorCallback(MessageCallback callback)
{
    error_callback_ = callback;
}

// Set an optional callback for associate responses
void Connector::setAssociateCallback(MessageCallback callback)
{
    associate_response_callback_ = callback;
}

// Set an optional callback for TTL expired messages
void Connector::setTTLExpiredCallback(MessageCallback callback)
{
    TTL_expired_callback_ = callback;
}

// Manage the connection state

void Connector::connect(int max_connect_attempts)
{
    if (connection_ptr_ == nullptr) {
        // Initialize the WebSocket connection
        connection_ptr_.reset(new Connection(broker_ws_uris_, client_metadata_));

        // Set WebSocket callbacks
        connection_ptr_->setOnMessageCallback(
            [this](std::string message) {
                processMessage(message);
            });

        connection_ptr_->setOnOpenCallback(
            [this]() {
                associateSession();
            });

        connection_ptr_->setOnCloseCallback(
            [this]() {
                closeAssociationTimings();
            });

        connection_ptr_->setOnFailCallback(
            [this]() {
                closeAssociationTimings();
            });
    }

    // Check the state of the Connection instance, to avoid waiting
    // for an Associate Session response in vain if a concurrent
    // attempt to open a WebSocket connection is in progress as, in
    // such case, Connection::connect() will not trigger the
    // transmission of any Associate Session request.
    // Note that we don't want to call associateSession()
    // synchronously, as the broker would close the WebSocket if the
    // session was already associated; such function must be invoked
    // only by the WebSocket onOpen handler, asynchronously. So, first
    // ensure that the connection is closed and then open it.
    switch (connection_ptr_->getConnectionState()) {
        case(ConnectionState::connecting):
        case(ConnectionState::open):
            LOG_DEBUG("There's an ongoing attempt to create a WebSocket connection; "
                      "ensuring that it's closed before Associate Session");
            try {
                connection_ptr_->close(CloseCodeValues::normal,
                                       "must Associate Session again");
                LOG_TRACE("Waiting for the WebSocket connection to be closed, "
                          "for a maximum of {1} s", WS_CONNECTION_CLOSE_TIMEOUT_S);
                lth_util::Timer timer {};

                while (connection_ptr_->getConnectionState()
                            != ConnectionState::closed
                       && timer.elapsed_seconds() < WS_CONNECTION_CLOSE_TIMEOUT_S)
                    lth_util::this_thread::sleep_for(lth_util::chrono::milliseconds(100));

                if (connection_ptr_->getConnectionState()
                        != ConnectionState::closed) {
                    LOG_WARNING("Unexpected - failed to close the WebSocket "
                                "connection");
                } else {
                    LOG_TRACE("The WebSocket connection is now closed");
                }
            } catch (connection_processing_error& e) {
                LOG_WARNING("WebSocket close failure ({1}); will continue with "
                            "the Associate Session attempt", e.what());
            }
            break;
        default:
            LOG_TRACE("There is no ongoing WebSocket connection; about to connect");
    }

    // TODO(ale): Version Negotiation impact on Session Association

    // Lock session_association_ in order to block the onOpen callback
    // (i.e. associateSession()) and prevent it from sending the
    // Associate Session request before Connection::connect() returns.
    // Otherwise, in case of Associate Session failure, a race between
    // 1) the onClose event (triggered by the broker, since it will
    // drop the WebSocket connection due to the failure) and 2) the
    // Connection's FSM (that checks the ConnectionState in periods of
    // CONNECTION_MIN_INTERVAL_MS) could leave pxp-agent retrying to
    // connect indefinitely (in case max_connect_attempts == 0)
    // instead of throwing a connection_association_response_failure.
    lth_util::unique_lock<lth_util::mutex> the_lock { session_association_.mtx };
    session_association_.reset();
    session_association_.in_progress = true;
    lth_util::chrono::seconds assoc_timeout { client_metadata_.association_timeout_s };

    try {
        // Open the WebSocket connection (blocking call)
        connection_ptr_->connect(max_connect_attempts);
        LOG_INFO("Waiting for the PCP Session Association to complete");
        session_association_.cond_var.wait_until(
            the_lock,                                           // lock
            lth_util::chrono::system_clock::now() + assoc_timeout,  // timeout
            [this]() -> bool {                                  // predicate
                return !session_association_.in_progress.load()
                        || session_association_.got_messaging_failure.load();
            });

        // Look for failures
        if (session_association_.got_messaging_failure.load()) {
            association_timings_.setCompleted(false);
            LOG_DEBUG("It seems that an error occurred during the Session "
                      "Association ({1}) - {2}",
                      (session_association_.error.empty()
                            ? lth_loc::translate("undetermined error")
                            : session_association_.error),
                       association_timings_.toString());
            session_association_.reset();
            throw connection_association_error {
                lth_loc::translate("invalid Associate Session response") };
        }
        if (session_association_.in_progress.load()) {
            // HERE(ale): don't set the associate_timings_ completion
            // as we can't be sure whether the request was sent
            LOG_DEBUG("Associate Session timed out");
            session_association_.reset();
            throw connection_association_error {
                lth_loc::translate("operation timeout") };
        }
        if (!session_association_.success.load()) {
            association_timings_.setCompleted(false);
            LOG_DEBUG(association_timings_.toString());
            std::string msg { lth_loc::translate("Associate Session failure") };
            if (!session_association_.error.empty())
                msg += ": " + session_association_.error;
            session_association_.reset();
            throw connection_association_response_failure { msg };
        }

        // Success!
        association_timings_.setCompleted();
        LOG_DEBUG(association_timings_.toString(false));
    } catch (const connection_processing_error& e) {
        // NB: connection_fatal_errors (can't connect after n tries)
        //     and _config_errors (TLS initialization error) are
        //     propagated whereas _processing_errors must be converted
        //     to _config_errors (they can be thrown after the
        //     synchronous call websocketpp::Endpoint::connect())
        LOG_DEBUG("Failed to establish the WebSocket connection ({1})", e.what());

        // NB: We don't update the association_timings here as we
        // can't be sure that the Association request was sent
        association_timings_.setCompleted(false);
        throw connection_config_error { e.what() };
    }
}

bool Connector::isConnected() const
{
    return connection_ptr_ != nullptr
           && connection_ptr_->getConnectionState() == ConnectionState::open;
}

bool Connector::isAssociated() const
{
    return isConnected() && session_association_.success.load();
}

ConnectionTimings Connector::getConnectionTimings() const
{
    return (connection_ptr_ == nullptr ? ConnectionTimings() : connection_ptr_->timings);
}

AssociationTimings Connector::getAssociationTimings() const
{
    return association_timings_;
}

static void checkPingTimings(uint32_t ping_interval_ms, uint32_t pong_timeout_ms)
{
    if (ping_interval_ms <= pong_timeout_ms) {
        throw connection_config_error {
            leatherman::locale::format("pong timeout ({1} ms) must be less "
                                       "than connection check interval ({2} ms)",
                                       pong_timeout_ms, ping_interval_ms) };
    }
}

void Connector::startMonitoring(const uint32_t max_connect_attempts,
                                const uint32_t connection_check_interval_s)
{
    checkConnectionInitialization();
    checkPingTimings(connection_check_interval_s*1000, client_metadata_.pong_timeout_ms);

    if (!is_monitoring_) {
        is_monitoring_ = true;
        monitor_thread_ = lth_util::thread { &Connector::startMonitorTask,
                                             this,
                                             max_connect_attempts,
                                             connection_check_interval_s };
    } else {
        LOG_WARNING("The Monitoring Thread is already running");
    }
}

void Connector::stopMonitoring()
{
    checkConnectionInitialization();

    if (is_monitoring_) {
        stopMonitorTaskAndWait();
    } else if (monitor_exception_) {
        LOG_DEBUG("The Monitoring Thread previously caught an exception; "
                  "re-throwing it");
        std::rethrow_exception(monitor_exception_);
    } else {
        LOG_WARNING("The Monitoring Thread is not running");
    }
}

void Connector::monitorConnection(const uint32_t max_connect_attempts,
                                  const uint32_t connection_check_interval_s)
{
    checkConnectionInitialization();
    checkPingTimings(connection_check_interval_s*1000, client_metadata_.pong_timeout_ms);

    if (!is_monitoring_) {
        is_monitoring_ = true;
        startMonitorTask(max_connect_attempts, connection_check_interval_s);

        // If startMonitorTask exits because of an exception, rethrow it now.
        // Avoid a race condition if the thread is stopped asynchronously by
        // checking must_stop_monitoring_.
        if (!must_stop_monitoring_ && monitor_exception_) {
            std::rethrow_exception(monitor_exception_);
        }
    } else {
        LOG_WARNING("The Monitoring Thread is already running");
    }
}

// Send messages

void Connector::send(const Message& msg)
{
    checkConnectionInitialization();
    auto serialized_msg = msg.getSerialized();
    LOG_DEBUG("Sending message of {1} bytes:\n{2}",
              serialized_msg.size(), msg.toString());
    connection_ptr_->send(&serialized_msg[0], serialized_msg.size());
}

std::string Connector::send(const std::vector<std::string>& targets,
                     const std::string& message_type,
                     unsigned int timeout,
                     const lth_jc::JsonContainer& data_json,
                     const std::vector<lth_jc::JsonContainer>& debug)
{
    return sendMessage(targets,
                       message_type,
                       timeout,
                       false,
                       data_json.toString(),
                       debug);
}

std::string Connector::send(const std::vector<std::string>& targets,
                     const std::string& message_type,
                     unsigned int timeout,
                     const std::string& data_binary,
                     const std::vector<lth_jc::JsonContainer>& debug)
{
    return sendMessage(targets,
                       message_type,
                       timeout,
                       false,
                       data_binary,
                       debug);
}

std::string Connector::send(const std::vector<std::string>& targets,
                     const std::string& message_type,
                     unsigned int timeout,
                     bool destination_report,
                     const lth_jc::JsonContainer& data_json,
                     const std::vector<lth_jc::JsonContainer>& debug)
{
    return sendMessage(targets,
                       message_type,
                       timeout,
                       destination_report,
                       data_json.toString(),
                       debug);
}

std::string Connector::send(const std::vector<std::string>& targets,
                     const std::string& message_type,
                     unsigned int timeout,
                     bool destination_report,
                     const std::string& data_binary,
                     const std::vector<lth_jc::JsonContainer>& debug)
{
    return sendMessage(targets,
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

void Connector::checkConnectionInitialization()
{
    if (connection_ptr_ == nullptr) {
        throw connection_not_init_error {
            lth_loc::translate("connection not initialized") };
    }
}

MessageChunk Connector::createEnvelope(const std::vector<std::string>& targets,
                                       const std::string& message_type,
                                       unsigned int timeout,
                                       bool destination_report,
                                       std::string& msg_id)
{
    msg_id = lth_util::get_UUID();
    auto expires = lth_util::get_ISO8601_time(timeout);
    // TODO(ale): deal with locale & plural (PCP-257)
    if (targets.size() == 1) {
        LOG_DEBUG("Creating message with id {1} for {2} receiver",
                  msg_id, targets.size());
    } else {
        LOG_DEBUG("Creating message with id {1} for {2} receivers",
                  msg_id, targets.size());
    }

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

std::string Connector::sendMessage(const std::vector<std::string>& targets,
                                   const std::string& message_type,
                                   unsigned int timeout,
                                   bool destination_report,
                                   const std::string& data_txt,
                                   const std::vector<lth_jc::JsonContainer>& debug)
{
    std::string msg_id {};
    auto envelope_chunk = createEnvelope(targets,
                                         message_type,
                                         timeout,
                                         destination_report,
                                         msg_id);
    MessageChunk data_chunk { ChunkDescriptor::DATA, data_txt };
    Message msg { envelope_chunk, data_chunk };

    for (auto debug_content : debug) {
        MessageChunk d_c { ChunkDescriptor::DEBUG, debug_content.toString() };
        msg.addDebugChunk(d_c);
    }

    send(msg);
    return msg_id;
}

//
// Callbacks
//

// WebSocket - onOpen callback (will send the PCP Associate Session request)

void Connector::associateSession()
{
    lth_util::lock_guard<lth_util::mutex> the_lock { session_association_.mtx };

    if (!session_association_.in_progress.load())
        LOG_DEBUG("About to send the Associate Session request; unexpectedly the "
                  "Connector does not seem to be in the associating state. "
                  "Note that the Association timings will be reset.");

    session_association_.got_messaging_failure = false;
    session_association_.error.clear();
    association_timings_.reset();

    // Envelope
    // NB: createEnvelope will update session_association_.request_id
    auto envelope = createEnvelope(std::vector<std::string> { MY_BROKER_URI },
                                   Protocol::ASSOCIATE_REQ_TYPE,
                                   client_metadata_.association_timeout_s,
                                   false,
                                   session_association_.request_id);

    // Create and send message
    // NB: don't report a possible failure to session_association_;
    //     just let the onOpen handler close the WebSocket connection
    //     and let a connection_association_error be triggered
    Message msg { envelope };
    LOG_INFO("Sending Associate Session request with id {1} and a TTL of {2} s",
             session_association_.request_id, client_metadata_.association_timeout_s);
    send(msg);
}

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
    } catch (const message_error& e) {
        err_msg = lth_loc::format("Failed to deserialize message: {1}", e.what());
    }

    // Parse message chunks, if the deserialization succeeded
    ParsedChunks parsed_chunks;

    if (err_msg.empty()) {
        try {
            parsed_chunks = msg_ptr->getParsedChunks(validator_);
        } catch (const lth_jc::validation_error& e) {
            err_msg = lth_loc::format("Invalid envelope - bad content: {1}", e.what());
        } catch (const lth_jc::data_parse_error& e) {
            err_msg = lth_loc::format("Invalid envelope - invalid JSON content: {1}",
                                      e.what());
        } catch (const lth_jc::schema_not_found_error& e) {
            // This is unexpected
            err_msg = lth_loc::format("Unknown schema: {1}", e.what());
        }
    }

    if (!err_msg.empty()) {
        // Log and return; we cannot break the WebSocket event loop
        LOG_ERROR(err_msg);
        if (session_association_.in_progress.load()) {
            // Report that a bad message was received, as
            // associateResponseCallback() won't be executed
            lth_util::lock_guard<lth_util::mutex> the_lock { session_association_.mtx };
            session_association_.got_messaging_failure = true;
            session_association_.error = err_msg;
            session_association_.cond_var.notify_one();
        }
        return;
    }

    // Execute the callback associated with the data schema
    auto schema_name = parsed_chunks.envelope.get<std::string>("message_type");

    if (schema_callback_pairs_.find(schema_name) != schema_callback_pairs_.end()) {
        auto c_b = schema_callback_pairs_.at(schema_name);
        LOG_TRACE("Executing callback for a message with '{1}' schema", schema_name);
        c_b(parsed_chunks);
    } else {
        LOG_WARNING("No message callback has be registered for the '{1}' schema",
                    schema_name);
    }
}

// WebSocket - onFail & onClose callback

void Connector::closeAssociationTimings()
{
    if (association_timings_.completed && !association_timings_.closed) {
        association_timings_.setClosed();
        LOG_DEBUG(association_timings_.toString());
    }
}

// PCP - Associate session response callback

void Connector::associateResponseCallback(const ParsedChunks& parsed_chunks)
{
    assert(parsed_chunks.has_data);
    assert(parsed_chunks.data_type == PCPClient::ContentType::Json);

    lth_util::lock_guard<lth_util::mutex> the_lock { session_association_.mtx };

    auto response_id = parsed_chunks.envelope.get<std::string>("id");
    auto sender_uri = parsed_chunks.envelope.get<std::string>("sender");
    auto success = parsed_chunks.data.get<bool>("success");
    auto request_id = parsed_chunks.data.get<std::string>("id");

    if (!session_association_.in_progress.load()) {
        LOG_WARNING("Received an unexpected Associate Session response; "
                    "discarding it");
        return;
    }

    if (request_id != session_association_.request_id) {
        LOG_WARNING("Received an Associate Session response that refers to an "
                    "unknown request ID ({1}, expected {2}); discarding it",
                    request_id, session_association_.request_id);
        return;
    }

    std::string msg { lth_loc::format("Received an Associate Session response "
                                      "{1} from {2} for the request {3}",
                                      response_id, sender_uri, request_id) };

    // HERE(ale): connect() is in charge of updating association_timings_
    // whereas the onOpen callback will reset it

    if (success) {
        LOG_INFO("{1}: success", msg);
    } else {
        if (parsed_chunks.data.includes("reason")) {
            session_association_.error = parsed_chunks.data.get<std::string>("reason");
            LOG_WARNING("{1}: failure - {2}", msg, session_association_.error);
        } else {
            session_association_.error.clear();
            LOG_WARNING("{1}: failure", msg);
        }
    }

    session_association_.success = success;
    session_association_.in_progress = false;

    if (associate_response_callback_)
        associate_response_callback_(parsed_chunks);

    session_association_.cond_var.notify_one();
}

// PCP - PCP Error message callback

void Connector::errorMessageCallback(const ParsedChunks& parsed_chunks)
{
    assert(parsed_chunks.has_data);
    assert(parsed_chunks.data_type == PCPClient::ContentType::Json);

    auto error_id = parsed_chunks.envelope.get<std::string>("id");
    auto sender_uri = parsed_chunks.envelope.get<std::string>("sender");
    auto description = parsed_chunks.data.get<std::string>("description");

    std::string cause_id {};
    std::string msg { lth_loc::format("Received error {1} from {2}",
                                      error_id, sender_uri) };

    if (parsed_chunks.data.includes("id")) {
        cause_id = parsed_chunks.data.get<std::string>("id");
        LOG_WARNING("{1} caused by message {2}: {3}", msg, cause_id, description);
    } else {
        LOG_WARNING("{1} (the id of the message that caused it is unknown): {2}",
                    msg, description);
    }

    if (error_callback_)
        error_callback_(parsed_chunks);

    if (session_association_.in_progress.load()) {
        lth_util::lock_guard<lth_util::mutex> the_lock { session_association_.mtx };

        if (!cause_id.empty() && cause_id == session_association_.request_id) {
            LOG_DEBUG("The error message {1} is due to the Associate Session "
                      "request {2}",
                      error_id, cause_id);
            // The Associate Session request caused the error
            session_association_.got_messaging_failure = true;
            session_association_.error = description;
            session_association_.cond_var.notify_one();
        }
    }
}

// PCP - ttl_expired message callback

void Connector::TTLMessageCallback(const ParsedChunks& parsed_chunks)
{
    assert(parsed_chunks.has_data);
    assert(parsed_chunks.data_type == PCPClient::ContentType::Json);

    auto ttl_msg_id = parsed_chunks.envelope.get<std::string>("id");
    auto expired_msg_id = parsed_chunks.data.get<std::string>("id");

    LOG_WARNING("Received TTL Expired message {1} from {2} related to message {3}",
                ttl_msg_id, parsed_chunks.envelope.get<std::string>("sender"),
                expired_msg_id);

    if (TTL_expired_callback_)
        TTL_expired_callback_(parsed_chunks);

    if (session_association_.in_progress.load()) {
        lth_util::lock_guard<lth_util::mutex> the_lock { session_association_.mtx };

        if (!expired_msg_id.empty()
                && expired_msg_id == session_association_.request_id) {
            LOG_DEBUG("The TTL expired message {1} is related to the Associate "
                      "Session request {2}",
                      ttl_msg_id, expired_msg_id);
            // The Associate Session request timed out
            session_association_.got_messaging_failure = true;
            session_association_.error = "Associate request's TTL expired";
            session_association_.cond_var.notify_one();
        }
    }
}

//
// Monitoring Task
//

void Connector::startMonitorTask(const uint32_t max_connect_attempts,
                                 const uint32_t connection_check_interval_s)
{
    assert(connection_ptr_ != nullptr);
    // Reset the exception, in case one was previously triggered and handled.
    monitor_exception_ = {};
    LOG_INFO("Starting the monitor task");
    lth_util::chrono::system_clock::time_point now {};
    lth_util::unique_lock<lth_util::mutex> the_lock { monitor_mutex_ };

    while (!must_stop_monitoring_) {
        now = lth_util::chrono::system_clock::now();

        monitor_cond_var_.wait_until(
            the_lock,
            now + lth_util::chrono::seconds(connection_check_interval_s));

        if (must_stop_monitoring_)
            break;

        try {
            if (!isConnected()) {
                LOG_WARNING("WebSocket connection to PCP broker lost; retrying");
                connect(max_connect_attempts);
            } else {
                LOG_DEBUG("Sending heartbeat ping");
                connection_ptr_->ping();
            }
        } catch (const connection_config_error& e) {
            // Connection::connect(), ping() or WebSocket TLS init
            // error - retry
            LOG_WARNING("The connection monitor task will continue after a "
                        "WebSocket failure ({1})", e.what());
        } catch (const connection_association_error& e) {
            // Associate Session timeout or invalid response - retry
            LOG_WARNING("The connection monitor task will continue after an "
                        "error during the Session Association ({1})", e.what());
        } catch (const connection_association_response_failure& e) {
            // Associate Session failure - stop
            LOG_WARNING("The connection monitor task will stop after an "
                        "Session Association failure: {1}", e.what());
            monitor_exception_ = std::current_exception();
            break;
        } catch (const connection_fatal_error& e) {
            // Failed to reconnect after max_connect_attempts - stop
            LOG_WARNING("The connection monitor task will stop: {1}", e.what());
            monitor_exception_ = std::current_exception();
            break;
        } catch (...) {
            // Make sure unexpected exceptions don't cause std::terminate yet,
            // and can be handled by the caller.
            monitor_exception_ = std::current_exception();
            break;
        }
    }

    LOG_INFO("Stopping the monitor task");
    is_monitoring_ = false;
}

void Connector::stopMonitorTaskAndWait() {
    LOG_INFO("Stopping the Monitoring Thread");
    must_stop_monitoring_ = true;
    monitor_cond_var_.notify_one();

    if (monitor_thread_.joinable()) {
        monitor_thread_.join();
    } else {
        LOG_WARNING("The Monitoring Thread is not joinable");
    }

    if (monitor_exception_) {
        std::rethrow_exception(monitor_exception_);
    }
}

}  // namespace PCPClient

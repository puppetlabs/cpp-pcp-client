#include <cpp-pcp-client/connector/connector_base.hpp>
#include <cpp-pcp-client/util/chrono.hpp>
#include <cpp-pcp-client/util/thread.hpp>

#define LEATHERMAN_LOGGING_NAMESPACE CPP_PCP_CLIENT_LOGGING_PREFIX".connector"

#include <leatherman/logging/logging.hpp>
#include <leatherman/locale/locale.hpp>

namespace PCPClient {

namespace lth_loc  = leatherman::locale;

const std::string ConnectorBase::MY_BROKER_URI { "pcp:///server" };

// legacy constructor: pre proxy
ConnectorBase::ConnectorBase(std::vector<std::string> broker_ws_uris,
                             std::string client_type,
                             std::string ca_crt_path,
                             std::string client_crt_path,
                             std::string client_key_path,
                             long ws_connection_timeout_ms,
                             uint32_t pong_timeouts_before_retry,
                             long ws_pong_timeout_ms)
        : connection_ptr_ { nullptr },
          broker_ws_uris_ { std::move(broker_ws_uris) },
          client_metadata_ { std::move(client_type),
                             std::move(ca_crt_path),
                             std::move(client_crt_path),
                             std::move(client_key_path),
                             std::move(ws_connection_timeout_ms),
                             std::move(pong_timeouts_before_retry),
                             std::move(ws_pong_timeout_ms)},
          validator_ {},
          schema_callback_pairs_ {},
          error_callback_ {},
          is_monitoring_ { false },
          monitor_thread_ {},
          monitor_mutex_ {},
          monitor_cond_var_ {},
          must_stop_monitoring_ { false }
{ }

// constructor for proxy addition
ConnectorBase::ConnectorBase(std::vector<std::string> broker_ws_uris,
                             std::string client_type,
                             std::string ca_crt_path,
                             std::string client_crt_path,
                             std::string client_key_path,
                             std::string ws_proxy,
                             long ws_connection_timeout_ms,
                             uint32_t pong_timeouts_before_retry,
                             long ws_pong_timeout_ms)
        : connection_ptr_ { nullptr },
          broker_ws_uris_ { std::move(broker_ws_uris) },
          client_metadata_ { std::move(client_type),
                             std::move(ca_crt_path),
                             std::move(client_crt_path),
                             std::move(client_key_path),
                             std::move(ws_proxy),
                             std::move(ws_connection_timeout_ms),
                             std::move(pong_timeouts_before_retry),
                             std::move(ws_pong_timeout_ms)},
          validator_ {},
          schema_callback_pairs_ {},
          error_callback_ {},
          is_monitoring_ { false },
          monitor_thread_ {},
          monitor_mutex_ {},
          monitor_cond_var_ {},
          must_stop_monitoring_ { false }
{ }

// constructor for crl addition
ConnectorBase::ConnectorBase(std::vector<std::string> broker_ws_uris,
                             std::string client_type,
                             std::string ca_crt_path,
                             std::string client_crt_path,
                             std::string client_key_path,
                             std::string client_crl_path,
                             std::string ws_proxy,
                             long ws_connection_timeout_ms,
                             uint32_t pong_timeouts_before_retry,
                             long ws_pong_timeout_ms)
        : connection_ptr_ { nullptr },
          broker_ws_uris_ { std::move(broker_ws_uris) },
          client_metadata_ { std::move(client_type),
                             std::move(ca_crt_path),
                             std::move(client_crt_path),
                             std::move(client_key_path),
                             std::move(client_crl_path),
                             std::move(ws_proxy),
                             std::move(ws_connection_timeout_ms),
                             std::move(pong_timeouts_before_retry),
                             std::move(ws_pong_timeout_ms)},
          validator_ {},
          schema_callback_pairs_ {},
          error_callback_ {},
          is_monitoring_ { false },
          monitor_thread_ {},
          monitor_mutex_ {},
          monitor_cond_var_ {},
          must_stop_monitoring_ { false }
{ }

ConnectorBase::~ConnectorBase()
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
            boost::rethrow_exception(monitor_exception_);
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Error previously caught by the Monitor Thread: {1}", e.what());
    } catch (...) {
        LOG_ERROR("An unexpected error has been previously caught by the Monitor Thread");
    }
}

// Register schemas and onMessage callbacks
void ConnectorBase::registerMessageCallback(const Schema& schema,
                                        MessageCallback callback)
{
    validator_.registerSchema(schema);
    auto p = std::pair<std::string, MessageCallback>(schema.getName(), callback);
    schema_callback_pairs_.insert(p);
}

// Set an optional callback for error messages
void ConnectorBase::setPCPErrorCallback(MessageCallback callback)
{
    error_callback_ = callback;
}

// Manage the connection state

void ConnectorBase::connect(int max_connect_attempts)
{
    if (connection_ptr_ == nullptr) {
        // Initialize the WebSocket connection
        connection_ptr_.reset(new Connection(broker_ws_uris_, client_metadata_));

        // Set WebSocket callbacks
        connection_ptr_->setOnMessageCallback(
            [this](std::string message) {
                processMessage(message);
            });

        connection_ptr_->setOnCloseCallback(
            [this]() {
                notifyClose();
            });
    }

    try {
        // Open the WebSocket connection (blocking call)
        connection_ptr_->connect(max_connect_attempts);
    } catch (const connection_processing_error& e) {
        // NB: connection_fatal_errors (can't connect after n tries)
        //     and _config_errors (TLS initialization error) are
        //     propagated whereas _processing_errors must be converted
        //     to _config_errors (they can be thrown after the
        //     synchronous call websocketpp::Endpoint::connect())
        LOG_DEBUG("Failed to establish the WebSocket connection ({1})", e.what());
        throw connection_config_error { e.what() };
    }
}

bool ConnectorBase::isConnected() const
{
    return connection_ptr_ != nullptr
           && connection_ptr_->getConnectionState() == ConnectionState::open;
}

ConnectionTimings ConnectorBase::getConnectionTimings() const
{
    return (connection_ptr_ == nullptr ? ConnectionTimings() : connection_ptr_->timings);
}

static void checkPingTimings(uint32_t ping_interval_ms, uint32_t pong_timeout_ms)
{
    if (ping_interval_ms <= pong_timeout_ms) {
        throw connection_config_error {
            lth_loc::format("pong timeout ({1} ms) must be less "
                            "than connection check interval ({2} ms)",
                            pong_timeout_ms, ping_interval_ms) };
    }
}

void ConnectorBase::startMonitoring(const uint32_t max_connect_attempts,
                                const uint32_t connection_check_interval_s)
{
    checkConnectionInitialization();
    checkPingTimings(connection_check_interval_s*1000, client_metadata_.pong_timeout_ms);

    if (!is_monitoring_) {
        is_monitoring_ = true;
        monitor_thread_ = Util::thread { &ConnectorBase::startMonitorTask,
                                         this,
                                         max_connect_attempts,
                                         connection_check_interval_s };
    } else {
        LOG_WARNING("The Monitoring Thread is already running");
    }
}

void ConnectorBase::stopMonitoring()
{
    checkConnectionInitialization();

    if (is_monitoring_) {
        stopMonitorTaskAndWait();
    } else if (monitor_exception_) {
        LOG_DEBUG("The Monitoring Thread previously caught an exception; "
                  "re-throwing it");
        boost::rethrow_exception(monitor_exception_);
    } else {
        LOG_WARNING("The Monitoring Thread is not running");
    }
}

void ConnectorBase::monitorConnection(const uint32_t max_connect_attempts,
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
            boost::rethrow_exception(monitor_exception_);
        }
    } else {
        LOG_WARNING("The Monitoring Thread is already running");
    }
}

// Utility functions

void ConnectorBase::checkConnectionInitialization()
{
    if (connection_ptr_ == nullptr) {
        throw connection_not_init_error {
            lth_loc::translate("connection not initialized") };
    }
}

void ConnectorBase::notifyClose()
{
    monitor_cond_var_.notify_one();
}

//
// Monitoring Task
//

void ConnectorBase::startMonitorTask(const uint32_t max_connect_attempts,
                                 const uint32_t connection_check_interval_s)
{
    assert(connection_ptr_ != nullptr);
    // Reset the exception, in case one was previously triggered and handled.
    monitor_exception_ = {};
    LOG_INFO("Starting the monitor task");
    Util::chrono::system_clock::time_point now {};
    Util::unique_lock<Util::mutex> the_lock { monitor_mutex_ };

    while (!must_stop_monitoring_) {
        now = Util::chrono::system_clock::now();

        monitor_cond_var_.wait_until(
            the_lock,
            now + Util::chrono::seconds(connection_check_interval_s));

        if (must_stop_monitoring_)
            break;

        try {
            if (!isConnected()) {
                LOG_WARNING("WebSocket connection to PCP broker lost; retrying");
                // Wait 200ms to avoid busy-waiting against pcp-broker if connections
                // are being closed.
                Util::this_thread::sleep_for(Util::chrono::milliseconds(200));
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
        } catch (const connection_processing_error& e) {
            // send or ping failed, likely connection lost - retry
            LOG_WARNING("The connection monitor task will continue after a "
                        "WebSocket failure ({1})", e.what());
        } catch (const connection_association_response_failure& e) {
            // Associate Session failure - stop
            LOG_WARNING("The connection monitor task will stop after an "
                        "Session Association failure: {1}", e.what());
            try {
                boost::throw_exception(e);
            } catch (...) {
                monitor_exception_ = boost::current_exception();
            }
            break;
        } catch (const connection_fatal_error& e) {
            // Failed to reconnect after max_connect_attempts - stop
            LOG_WARNING("The connection monitor task will stop: {1}", e.what());
            try {
                boost::throw_exception(e);
            } catch (...) {
                monitor_exception_ = boost::current_exception();
            }
            break;
        } catch (const std::exception& e) {
            // Make sure unexpected exceptions don't cause std::terminate yet,
            // and can be handled by the caller.
            LOG_WARNING("The connection monitor task encountered an unknown "
                        "exception: {1}", e.what());
            try {
                boost::throw_exception(e);
            } catch (...) {
                monitor_exception_ = boost::current_exception();
            }
            break;
        }
    }

    LOG_INFO("Stopping the monitor task");
    is_monitoring_ = false;
}

void ConnectorBase::stopMonitorTaskAndWait() {
    LOG_INFO("Stopping the Monitoring Thread");
    must_stop_monitoring_ = true;
    monitor_cond_var_.notify_one();

    if (monitor_thread_.joinable()) {
        monitor_thread_.join();
    } else {
        LOG_WARNING("The Monitoring Thread is not joinable");
    }

    if (monitor_exception_) {
        boost::rethrow_exception(monitor_exception_);
    }
}

}  // namespace PCPClient

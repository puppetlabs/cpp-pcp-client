#pragma once

#include <cpp-pcp-client/connector/connection.hpp>
#include <cpp-pcp-client/connector/client_metadata.hpp>
#include <cpp-pcp-client/connector/errors.hpp>
#include <cpp-pcp-client/connector/timings.hpp>

#include <cpp-pcp-client/protocol/parsed_chunks.hpp>

#include <cpp-pcp-client/validator/validator.hpp>
#include <cpp-pcp-client/validator/schema.hpp>

#include <cpp-pcp-client/util/thread.hpp>

#include <cpp-pcp-client/export.h>

namespace PCPClient {

//
// ConnectorBase
//
// A base class for shared implemention between v1 and v2 connectors.
//

class LIBCPP_PCP_CLIENT_EXPORT ConnectorBase {
  public:
    using MessageCallback = std::function<void(const ParsedChunks&)>;

    ConnectorBase() = delete;
    // legacy constructor: pre proxy
    ConnectorBase(std::vector<std::string> broker_ws_uris,
        std::string client_type,
        std::string ca_crt_path,
        std::string client_crt_path,
        std::string client_key_path,
        long ws_connection_timeout_ms,
        uint32_t pong_timeouts_before_retry,
        long ws_pong_timeout_ms);

    // constructor proxy addition
    ConnectorBase(std::vector<std::string> broker_ws_uris,
        std::string client_type,
        std::string ca_crt_path,
        std::string client_crt_path,
        std::string client_key_path,
        std::string ws_proxy,
        long ws_connection_timeout_ms,
        uint32_t pong_timeouts_before_retry,
        long ws_pong_timeout_ms);

    // constructor crl addition
    ConnectorBase(std::vector<std::string> broker_ws_uris,
        std::string client_type,
        std::string ca_crt_path,
        std::string client_crt_path,
        std::string client_key_path,
        std::string client_crl_path,
        std::string ws_proxy,
        long ws_connection_timeout_ms,
        uint32_t pong_timeouts_before_retry,
        long ws_pong_timeout_ms);

    /// Calls stopMonitorTaskAndWait if the Monitoring Task thread is
    /// still active. In case an exception was previously stored by
    /// the Monitoring Task, the error message will be logged, but
    /// the exception won't be rethrown.
    virtual ~ConnectorBase();

    /// Throw a schema_redefinition_error if the specified schema has
    /// been already registred.
    void registerMessageCallback(const Schema& schema,
                                 MessageCallback callback);

    /// Set an optional callback for error messages
    void setPCPErrorCallback(MessageCallback callback);

    /// Open the WebSocket connection
    ///
    /// Check the state of the underlying connection (WebSocket); in
    /// case it's 'open' or 'connecting', close it. Then open it.
    /// Try to (re)open for max_connect_attempts times or
    /// indefinitely, in case that parameter is 0 (as by default).
    /// This is done by following an exponential backoff.
    /// Consider possible PCP Error messages that occur on connection.
    ///
    /// Throw a connection_config_error if it fails to set up the
    /// underlying communications layer (ex. invalid certificates).
    ///
    /// Throw a connection_fatal_error if it fails to open the
    /// underlying connection after the specified number of attempts
    /// (ex. the specified broker is down or rejecting connections).
    ///
    /// NB: this function is not thread safe; it should not be called
    ///     when the monitor thread is executing
    virtual void connect(int max_connect_attempts = 0);

    /// Returns true if the underlying connection is currently open,
    /// false otherwise.
    bool isConnected() const;

    /// Returns the timings of the underlying WebSocket connection
    /// if established, otherwise invokes and returns the
    /// ConnectionTimings' default constructor.
    ConnectionTimings getConnectionTimings() const;

    /// Starts the Monitoring Task in a separate thread.
    /// Such task will periodically check the state of the
    /// underlying connection and re-establish it in case it has
    /// dropped, otherwise it will send a WebSocket ping to the
    /// current broker in to keep the connection alive.
    /// The check period is specified by connection_check_interval_s
    /// (optional, in seconds).
    /// The max_connect_attempts parameters is used to reconnect
    /// (optional); it works as for the above connect() function.
    ///
    /// Note that startMonitoring simply returns in case of multiple
    /// calls.
    ///
    /// It is safe to call startMonitoring in a multithreading
    /// context; it will return as soon as the dtor has been invoked.
    ///
    /// Throws a connection_not_init_error in case the connection has
    /// not been opened previously.
    ///
    /// Propagates a possible exception thrown while spawning the
    /// Monitor Thread.
    ///
    /// Exceptions caught by the Monitoring Task may be stored
    /// and propagated later, by a subsequent call to stopMonitoring
    /// or the destructor. In that case, the task will terminate.
    /// Otherwise, in case the exception is not stored, the error
    /// will be logged as a warning.
    /// The following exceptions are simply logged:
    ///   - connection_config_error (invalid certificates);
    ///   - connection_association_error (failure during the Association
    ///     on v1 connections);
    /// The following exceptions are stored:
    ///   - connection_association_response_failure (the Association
    ///     response was not successful on v1 connections);
    ///   - connection_fatal_error (failed to re-connect after the
    ///     specified maximum number of attempts);
    ///   - all other exceptions.
    void startMonitoring(const uint32_t max_connect_attempts = 0,
                         const uint32_t connection_check_interval_s = 15);

    /// Stops the Monitoring Task in case it's running, otherwise the
    /// function simply logs a warning.
    ///
    /// Throws a connection_not_init_error in case the connection has
    /// not been opened previously.
    /// Rethrows a possible exception stored by the Monitoring Task
    /// (see startMonitoring doc above).
    void stopMonitoring();

    /// A blocking version of startMonitoring.
    ///
    /// Throws a connection_not_init_error in case the connection has
    /// not been opened previously.
    /// Rethrows a possible exception stored by the Monitoring Task
    /// (see the startMonitoring doc above).
    void monitorConnection(const uint32_t max_connect_attempts = 0,
                           const uint32_t connection_check_interval_s = 15);

  protected:
    static const std::string MY_BROKER_URI;

    /// Connection instance pointer
    std::unique_ptr<Connection> connection_ptr_;

    /// WebSocket URIs of PCP brokers; first entry is the default
    std::vector<std::string> broker_ws_uris_;

    /// Client metadata
    ClientMetadata client_metadata_;

    /// Content validator
    Validator validator_;

    /// Schema - onMessage callback map
    std::map<std::string, MessageCallback> schema_callback_pairs_;

    /// Error callback
    MessageCallback error_callback_;

    void checkConnectionInitialization();

    // WebSocket Callback for the Connection instance to handle all
    // incoming messages.
    // Parse and validate the passed message; execute the callback
    // associated with the schema specified in the envelope.
    //
    // Implement to define how incoming messages are handled.
    virtual void processMessage(const std::string& msg_txt) = 0;

    /// Used to notify the Monitoring Task when a connection is lost
    void notifyClose();

  private:
    /// Flag; true if monitorConnection is executing
    bool is_monitoring_;

    /// To manage the Monitoring Task
    Util::thread monitor_thread_;
    Util::mutex monitor_mutex_;
    Util::condition_variable monitor_cond_var_;
    bool must_stop_monitoring_;
    Util::exception_ptr monitor_exception_;

    // Monitor the underlying connection; reconnect or keep it alive.
    // If the underlying connection is dropped, unset the
    // is_associated_ flag.
    void startMonitorTask(const uint32_t max_connect_attempts,
                          const uint32_t connection_check_interval_s);

    // Stop the monitoring thread and wait for it to terminate.
    // Then rethrows any exceptions encountered while startMonitorTask
    // was running in its own thread.
    void stopMonitorTaskAndWait();
};

}  // namespace PCPClient

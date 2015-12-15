#ifndef CPP_PCP_CLIENT_SRC_CONNECTOR_CONNECTOR_H_
#define CPP_PCP_CLIENT_SRC_CONNECTOR_CONNECTOR_H_

#include <cpp-pcp-client/connector/connection.hpp>
#include <cpp-pcp-client/connector/client_metadata.hpp>
#include <cpp-pcp-client/connector/errors.hpp>
#include <cpp-pcp-client/validator/validator.hpp>
#include <cpp-pcp-client/validator/schema.hpp>
#include <cpp-pcp-client/protocol/chunks.hpp>
#include <cpp-pcp-client/protocol/message.hpp>
#include <cpp-pcp-client/util/thread.hpp>
#include <cpp-pcp-client/export.h>

#include <memory>
#include <string>
#include <map>
#include <atomic>

namespace PCPClient {

//
// Connector
//
namespace lth_jc = leatherman::json_container;

class LIBCPP_PCP_CLIENT_EXPORT Connector {
  public:
    using MessageCallback = std::function<void(const ParsedChunks& parsed_chunks)>;

    Connector() = delete;

    /// The timeout for the call that establishes the WebSocket
    /// connection is set, by default, to 5000 ms.
    /// Throws a connection_config_error in case the client
    /// certificate file does not exist or is invalid; it fails to
    /// retrieve the client identity from the file; the client
    /// certificate and private key are not paired
    Connector(const std::string& broker_ws_uri,
              const std::string& client_type,
              const std::string& ca_crt_path,
              const std::string& client_crt_path,
              const std::string& client_key_path,
              const long& connection_timeout = 5000);

    ~Connector();

    /// Throw a schema_redefinition_error if the specified schema has
    /// been already registred.
    void registerMessageCallback(const Schema schema,
                                 MessageCallback callback);

    /// Set an optional callback for error messages
    void setPCPErrorCallback(MessageCallback callback);

    /// Set an optional callback for associate responses
    void setAssociateCallback(MessageCallback callback);

    /// Check the state of the underlying connection (WebSocket); in
    /// case it's not open, try to re-open it.
    /// Try to reopen for max_connect_attempts times or idefinetely,
    /// in case that parameter is 0 (as by default). This is done by
    /// following an exponential backoff.
    /// Once the underlying connection is open, send an associate
    /// session request to the broker.
    /// Throw a connection_config_error if it fails to set up the
    /// underlying communications layer (ex. invalid certificates).
    /// Throw a connection_fatal_error if it fails to open the
    /// underlying connection after the specified number of attempts
    /// or if it fails to send the associate session request.
    /// NB: the function does not wait for the associate response; it
    ///     returns right after sending the associate request
    /// NB: the function is not thread safe
    void connect(int max_connect_attempts = 0);

    /// Returns true if the underlying connection is currently open,
    /// false otherwise.
    bool isConnected() const;

    /// Returns true if a successful associate response has been
    /// received and the underlying connection did not drop since
    /// then, false otherwise.
    bool isAssociated() const;

    /// Periodically check the state of the underlying connection.
    /// Re-establish the connection in case it has dropped, otherwise
    /// send a ping mst to the broker in order to keep the connection
    /// alive.
    /// The max_connect_attempts parameters is used to reconnect; it
    /// works as for the above connect() function.
    ///
    /// Note that this is a blocking call; this task will not be
    /// executed in a separate thread.
    /// Also, monitorConnection simply returns in case of multiple
    /// calls.
    ///
    /// It is safe to call monitorConnection in a multithreading
    /// context; it will return as soon as the dtor has been invoked.
    ///
    /// Throw a connection_fatal_error if, in case of dropped
    /// underlying connection, it fails to re-connect after the
    /// specified maximum number of attempts.
    /// Throws a connection_not_init_error in case the connection has
    /// not been opened previously.
    void monitorConnection(int max_connect_attempts = 0);

    /// Send the specified message.
    ///
    /// Other overloads may specify:
    ///   - targets: list of PCP URI strings
    ///   - message_type: schema name that identifies the message type
    ///   - timeout: expires entry in seconds
    ///   - destination_report: bool; the client must flag it in case
    ///     he wants to receive a destination report from the broker
    ///   - data: in binary (string)
    ///
    /// All methods:
    ///   - throw a connection_processing_error in case of failure;
    ///   - throw a connection_not_init_error in case the connection
    ///     has not been opened previously.
    void send(const Message& msg);

    void send(const std::vector<std::string>& targets,
              const std::string& message_type,
              unsigned int timeout,
              const lth_jc::JsonContainer& data_json,
              const std::vector<lth_jc::JsonContainer>& debug
                        = std::vector<lth_jc::JsonContainer> {});

    void send(const std::vector<std::string>& targets,
              const std::string& message_type,
              unsigned int timeout,
              const std::string& data_binary,
              const std::vector<lth_jc::JsonContainer>& debug
                        = std::vector<lth_jc::JsonContainer> {});

    void send(const std::vector<std::string>& targets,
              const std::string& message_type,
              unsigned int timeout,
              bool destination_report,
              const lth_jc::JsonContainer& data_json,
              const std::vector<lth_jc::JsonContainer>& debug
                        = std::vector<lth_jc::JsonContainer> {});

    void send(const std::vector<std::string>& targets,
              const std::string& message_type,
              unsigned int timeout,
              bool destination_report,
              const std::string& data_binary,
              const std::vector<lth_jc::JsonContainer>& debug
                        = std::vector<lth_jc::JsonContainer> {});

  private:
    /// WebSocket URI of the PCP broker
    std::string broker_ws_uri_;

    /// Client metadata
    ClientMetadata client_metadata_;

    /// Connection instance pointer
    std::unique_ptr<Connection> connection_ptr_;

    /// Content validator
    Validator validator_;

    /// Schema - onMessage callback map
    std::map<std::string, MessageCallback> schema_callback_pairs_;

    /// Error callback
    MessageCallback error_callback_;

    /// Associate response callback
    MessageCallback associate_response_callback_;

    /// Flag; true if monitorConnection is executing
    bool is_monitoring_;

    /// To manage the monitoring task
    Util::mutex mutex_;
    Util::condition_variable cond_var_;

    /// Flag; set to true if the dtor has been called
    bool is_destructing_;

    /// Flag; set to true when a successful associate response is
    /// received, set to false by the ctor or when the underlying
    /// connection drops.
    std::atomic<bool> is_associated_;

    void checkConnectionInitialization();

    MessageChunk createEnvelope(const std::vector<std::string>& targets,
                                const std::string& message_type,
                                unsigned int timeout,
                                bool destination_report);

    void sendMessage(const std::vector<std::string>& targets,
                     const std::string& message_type,
                     unsigned int timeout,
                     bool destination_report,
                     const std::string& data_txt,
                     const std::vector<lth_jc::JsonContainer>& debug);

    // WebSocket Callback for the Connection instance to be triggered
    // on an onOpen event.
    void associateSession();

    // WebSocket Callback for the Connection instance to handle all
    // incoming messages.
    // Parse and validate the passed message; execute the callback
    // associated with the schema specified in the envelope.
    void processMessage(const std::string& msg_txt);

    // PCP Callback executed by processMessage in case of an
    // associate session response.
    void associateResponseCallback(const PCPClient::ParsedChunks& parsed_chunks);

    // PCP Callback executed by processMessage when an error message
    // is received.
    void errorMessageCallback(const PCPClient::ParsedChunks& parsed_chunks);

    // Monitor the underlying connection; reconnect or keep it alive.
    // If the underlying connection is dropped, unset the
    // is_associated_ flag.
    void startMonitorTask(int max_connect_attempts);
};

}  // namespace PCPClient

#endif  // CPP_PCP_CLIENT_SRC_CONNECTOR_CONNECTOR_H_

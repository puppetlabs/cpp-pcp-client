#ifndef CTHUN_CLIENT_SRC_CONNECTOR_CONNECTOR_H_
#define CTHUN_CLIENT_SRC_CONNECTOR_CONNECTOR_H_

#include <cthun-client/connector/connection.hpp>
#include <cthun-client/connector/client_metadata.hpp>
#include <cthun-client/connector/errors.hpp>
#include <cthun-client/validator/validator.hpp>
#include <cthun-client/validator/schema.hpp>
#include <cthun-client/protocol/chunks.hpp>
#include <cthun-client/protocol/message.hpp>

#include <memory>
#include <string>
#include <map>
#include <thread>  // mutex
#include <condition_variable>
#include <atomic>

namespace CthunClient {

//
// Connector
//
namespace LTH_JC = leatherman::json_container;

class Connector {
  public:
    using MessageCallback = std::function<void(const ParsedChunks& parsed_chunks)>;

    Connector() = delete;

    /// Throws a connection_config_error in case it fails to retrieve
    /// the client identity from its certificate.
    Connector(const std::string& server_url,
              const std::string& client_type,
              const std::string& ca_crt_path,
              const std::string& client_crt_path,
              const std::string& client_key_path);

    ~Connector();

    /// Throw a schema_redefinition_error if the specified schema has
    /// been already registred.
    void registerMessageCallback(const Schema schema,
                                 MessageCallback callback);

    /// Set an optional callback for error messages
    void setCthunErrorCallback(MessageCallback callback);

    /// Set an optional callback for associate responses
    void setAssociateCallback(MessageCallback callback);

    /// Check the state of the underlying connection (WebSocket); in
    /// case it's not open, try to re-open it.
    /// Try to reopen for max_connect_attempts times or idefinetely,
    /// in case that parameter is 0 (as by default). This is done by
    /// following an exponential backoff.
    /// Once the underlying connection is open, send an associate
    /// session request to the server.
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
    /// send a ping mst to the server in order to keep the connection
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

    /// Send a message.
    /// Throw a connection_processing_error in case of failure.
    /// Throw a connection_not_init_error in case the connection has
    /// not been opened previously.
    void send(const Message& msg);

    void send(const std::vector<std::string>& targets,
              const std::string& message_type,
              unsigned int timeout,
              const LTH_JC::JsonContainer& data_json,
              const std::vector<LTH_JC::JsonContainer>& debug = std::vector<LTH_JC::JsonContainer> {});

    void send(const std::vector<std::string>& targets,
              const std::string& message_type,
              unsigned int timeout,
              const std::string& data_binary,
              const std::vector<LTH_JC::JsonContainer>& debug = std::vector<LTH_JC::JsonContainer> {});

    void send(const std::vector<std::string>& targets,
              const std::string& message_type,
              unsigned int timeout,
              bool destination_report,
              const LTH_JC::JsonContainer& data_json,
              const std::vector<LTH_JC::JsonContainer>& debug = std::vector<LTH_JC::JsonContainer> {});

    void send(const std::vector<std::string>& targets,
              const std::string& message_type,
              unsigned int timeout,
              bool destination_report,
              const std::string& data_binary,
              const std::vector<LTH_JC::JsonContainer>& debug = std::vector<LTH_JC::JsonContainer> {});

  private:
    /// Cthun server url
    std::string server_url_;

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
    std::mutex mutex_;
    std::condition_variable cond_var_;

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
                     const std::vector<LTH_JC::JsonContainer>& debug);

    // WebSocket Callback for the Connection instance to be triggered
    // on an onOpen event.
    void associateSession();

    // WebSocket Callback for the Connection instance to handle all
    // incoming messages.
    // Parse and validate the passed message; execute the callback
    // associated with the schema specified in the envelope.
    void processMessage(const std::string& msg_txt);

    // Cthun Callback executed by processMessage in case of an
    // associate session response.
    void associateResponseCallback(const CthunClient::ParsedChunks& parsed_chunks);

    // Cthun Callback executed by processMessage when an error message
    // is received.
    void errorMessageCallback(const CthunClient::ParsedChunks& parsed_chunks);

    // Monitor the underlying connection; reconnect or keep it alive.
    // If the underlying connection is dropped, unset the
    // is_associated_ flag.
    void startMonitorTask(int max_connect_attempts);
};

}  // namespace CthunClient

#endif  // CTHUN_CLIENT_SRC_CONNECTOR_CONNECTOR_H_

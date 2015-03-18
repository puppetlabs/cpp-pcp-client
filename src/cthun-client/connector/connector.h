#ifndef CTHUN_CLIENT_SRC_CONNECTOR_CONNECTOR_H_
#define CTHUN_CLIENT_SRC_CONNECTOR_CONNECTOR_H_

#include "./connection.h"
#include "./client_metadata.h"

#include "../validator/validator.h"
#include "../validator/schema.h"

#include "../message/chunks.h"
#include "../message/message.h"

#include "../data_container/data_container.h"

#include <memory>
#include <string>
#include <map>
#include <thread>  // mutex
#include <condition_variable>

namespace CthunClient {

//
// Connector
//

class Connector {
  public:
    using MessageCallback = std::function<void(const ParsedChunks& parsed_chunks)>;

    Connector() = delete;

    /// Throws a connection_config_error in case it fails to retrieve
    /// the client identity from its certificate.
    Connector(const std::string& server_url,
              const std::string& type,
              const std::string& ca_crt_path,
              const std::string& client_crt_path,
              const std::string& client_key_path);

    ~Connector();

    /// Throw a schema_redefinition_error if the specified schema has
    /// been already registred.
    void registerMessageCallback(const Schema schema,
                                 MessageCallback callback);

    /// Check the state of the underlying connection (WebSocket); in
    /// case it's not open, try to re-open it.
    /// Try to reopen for max_connect_attempts times or idefinetely,
    /// in case that parameter is 0 (as by default). This is done by
    /// following an exponential backoff.
    /// Once the underlying connection is open, send a login message
    /// to the server.
    /// Throw a connection_config_error if it fails to set up the
    /// underlying communications layer (ex. invalid certificates).
    /// Throw a connection_fatal_error if it fails to open the
    /// underlying connection after the specified number of attempts
    /// or if it fails to send the login message.
    /// NB: the function does not wait for the login response; it
    ///     returns right after sending the login request
    /// NB: the function is not thread safe
    void connect(int max_connect_attempts = 0);

    // TODO(ale): logged-in flag to be set after a login response

    bool isConnected() const;

    // HERE(ale): monitorConnection assumes that the client will
    // still be logged in once the WebSocket connection is re-opened

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

    void send(const std::vector<std::string>& endpoints,
              const std::string& data_schema,
              unsigned int timeout,
              const DataContainer& data_json,
              const std::vector<DataContainer>& debug = std::vector<DataContainer> {});

    void send(const std::vector<std::string>& endpoints,
              const std::string& data_schema,
              unsigned int timeout,
              const std::string& data_binary,
              const std::vector<DataContainer>& debug = std::vector<DataContainer> {});

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

    /// Flag; true if monitorConnection is executing
    bool is_monitoring_;

    /// To manage the monitoring task
    std::mutex mutex_;
    std::condition_variable cond_var_;

    /// Flag; set to true if the dtor has been called
    bool is_destructing_;

    void checkConnectionInitialization();

    void addEnvelopeSchemaToValidator();

    MessageChunk createEnvelope(const std::vector<std::string>& endpoints,
                                const std::string& data_schema,
                                unsigned int timeout);

    void sendMessage(const std::vector<std::string>& endpoints,
                     const std::string& data_schema,
                     unsigned int timeout,
                     const std::string& data_txt,
                     const std::vector<DataContainer>& debug);

    void sendLogin();


    // Callback for the Connection instance to handle incoming
    // messages.
    // Parse and validate the passed message; execute the callback
    // associated with the schema specified in the envelope.
    void processMessage(const std::string& msg_txt);

    // Monitor the underlying connection; reconnect or keep it alive
    void startMonitorTask(int max_connect_attempts);
};

}  // namespace CthunClient

#endif  // CTHUN_CLIENT_SRC_CONNECTOR_CONNECTOR_H_

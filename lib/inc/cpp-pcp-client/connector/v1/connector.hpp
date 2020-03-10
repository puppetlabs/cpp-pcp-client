#pragma once

#include <cpp-pcp-client/connector/v1/session_association.hpp>
#include <cpp-pcp-client/protocol/v1/message.hpp>

#include <cpp-pcp-client/connector/connector_base.hpp>

#include <memory>
#include <string>
#include <map>
#include <cstdint>

namespace PCPClient {
namespace v1 {

//
// Connector
//

class LIBCPP_PCP_CLIENT_EXPORT Connector : public ConnectorBase {
  public:
    Connector() = delete;

    /// The timeout for the call that establishes the WebSocket
    /// connection is set, by default, to 5000 ms.
    /// Throws a connection_config_error in case the client
    /// certificate file does not exist or is invalid; it fails to
    /// retrieve the client identity from the file; the client
    /// certificate and private key are not paired

    // legacy constructor: pre proxy
    Connector(std::string broker_ws_uri,
              std::string client_type,
              std::string ca_crt_path,
              std::string client_crt_path,
              std::string client_key_path,
              long ws_connection_timeout_ms = 5000,
              uint32_t association_timeout_s = 15,
              uint32_t association_request_ttl_s = 10,  // Unused
              uint32_t pong_timeouts_before_retry = 3,
              long ws_pong_timeout_ms = 5000);

    // constructor for proxy addition
    Connector(std::string broker_ws_uri,
              std::string client_type,
              std::string ca_crt_path,
              std::string client_crt_path,
              std::string client_key_path,
              std::string ws_proxy,
              long ws_connection_timeout_ms = 5000,
              uint32_t association_timeout_s = 15,
              uint32_t association_request_ttl_s = 10,  // Unused
              uint32_t pong_timeouts_before_retry = 3,
              long ws_pong_timeout_ms = 5000);

    // constructor for crl addition
    Connector(std::string broker_ws_uri,
              std::string client_type,
              std::string ca_crt_path,
              std::string client_crt_path,
              std::string client_key_path,
              std::string client_crl_path,
              std::string ws_proxy,
              long ws_connection_timeout_ms = 5000,
              uint32_t association_timeout_s = 15,
              uint32_t association_request_ttl_s = 10,  // Unused
              uint32_t pong_timeouts_before_retry = 3,
              long ws_pong_timeout_ms = 5000);

    // legacy constructor: pre proxy
    Connector(std::vector<std::string> broker_ws_uris,
              std::string client_type,
              std::string ca_crt_path,
              std::string client_crt_path,
              std::string client_key_path,
              long ws_connection_timeout_ms = 5000,
              uint32_t association_timeout_s = 15,
              uint32_t association_request_ttl_s = 10,  // Unused
              uint32_t pong_timeouts_before_retry = 3,
              long ws_pong_timeout_ms = 5000);

    // constructor for proxy addition
    Connector(std::vector<std::string> broker_ws_uris,
              std::string client_type,
              std::string ca_crt_path,
              std::string client_crt_path,
              std::string client_key_path,
              std::string ws_proxy,
              long ws_connection_timeout_ms = 5000,
              uint32_t association_timeout_s = 15,
              uint32_t association_request_ttl_s = 10,  // Unused
              uint32_t pong_timeouts_before_retry = 3,
              long ws_pong_timeout_ms = 5000);

    // constructor for crl addition
    Connector(std::vector<std::string> broker_ws_uris,
              std::string client_type,
              std::string ca_crt_path,
              std::string client_crt_path,
              std::string client_key_path,
              std::string client_crl_path,
              std::string ws_proxy,
              long ws_connection_timeout_ms = 5000,
              uint32_t association_timeout_s = 15,
              uint32_t association_request_ttl_s = 10,  // Unused
              uint32_t pong_timeouts_before_retry = 3,
              long ws_pong_timeout_ms = 5000);

    /// Set an optional callback for associate responses
    void setAssociateCallback(MessageCallback callback);

    /// Set an optional callback for TTL expired messages
    void setTTLExpiredCallback(MessageCallback callback);

    /// Open the WebSocket connection and perform Session Association
    ///
    /// Check the state of the underlying connection (WebSocket); in
    /// case it's 'open' or 'connecting', close it. Then open it and
    /// perform the PCP Session Association.
    /// Try to (re)open for max_connect_attempts times or
    /// indefinitely, in case that parameter is 0 (as by default).
    /// This is done by following an exponential backoff.
    /// Once the underlying connection is open, send an Associate
    /// Session request to the broker (asynchronously; done by the
    /// onOpen handler). Wait until an Associate Session response is
    /// received and process it; a timeout will be used for that - see
    /// error section below. In the meantime, consider any invalid
    /// message received as a possible corrupted Associate Session
    /// response. Also, consider possible PCP Error messages.
    ///
    /// Throw a connection_config_error if it fails to set up the
    /// underlying communications layer (ex. invalid certificates).
    ///
    /// Throw a connection_fatal_error if it fails to open the
    /// underlying connection after the specified number of attempts
    /// (ex. the specified broker is down).
    ///
    /// Throw a connection_association_error if:
    ///  - an invalid message is received;
    ///  - a PCP Error message is received, related to the current
    ///    Associate Session request (matching done by message ID);
    ///  - a TTL Expired message is received, related to the current
    ///    Associate Session request (matching done by message ID);
    ///  - an Associate Session response is not received after a time
    ///    interval equal to the ping period of monitorConnection()
    ///    (15 s) after sending the Associate Session request.
    ///
    /// Throw a connection_association_response_failure if the
    /// received associate session response does not indicate a
    /// success.
    ///
    /// NB: this function is not thread safe; it should not be called
    ///     when the monitor thread is executing
    void connect(int max_connect_attempts = 0) override;

    /// Returns true if a successful Associate response has been
    /// received and the underlying connection did not drop since
    /// then, false otherwise.
    bool isAssociated() const;

    /// Returns the timings of the PCP Associate Session.
    AssociationTimings getAssociationTimings() const;

    /// Send the specified message.
    /// Throw a connection_processing_error in case of failure;
    /// throw a connection_not_init_error in case the connection
    /// has not been opened previously.
    void send(const Message& msg);

    /// send() overloads that create and send a message as specified.
    /// Return the ID of the message, as a string.
    ///
    /// The caller may specify:
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
    std::string send(const std::vector<std::string>& targets,
                     const std::string& message_type,
                     unsigned int timeout,
                     const lth_jc::JsonContainer& data_json,
                     const std::vector<lth_jc::JsonContainer>& debug
                        = std::vector<lth_jc::JsonContainer> {});

    std::string send(const std::vector<std::string>& targets,
                     const std::string& message_type,
                     unsigned int timeout,
                     const std::string& data_binary,
                     const std::vector<lth_jc::JsonContainer>& debug
                        = std::vector<lth_jc::JsonContainer> {});

    std::string send(const std::vector<std::string>& targets,
                     const std::string& message_type,
                     unsigned int timeout,
                     bool destination_report,
                     const lth_jc::JsonContainer& data_json,
                     const std::vector<lth_jc::JsonContainer>& debug
                        = std::vector<lth_jc::JsonContainer> {});

    std::string send(const std::vector<std::string>& targets,
                     const std::string& message_type,
                     unsigned int timeout,
                     bool destination_report,
                     const std::string& data_binary,
                     const std::vector<lth_jc::JsonContainer>& debug
                        = std::vector<lth_jc::JsonContainer> {});

    std::string sendError(const std::vector<std::string>& targets,
                          unsigned int timeout,
                          const std::string& id,
                          const std::string& description);

  protected:
    // WebSocket Callback for the Connection instance to handle all
    // incoming messages.
    // Parse and validate the passed message; execute the callback
    // associated with the schema specified in the envelope.
    void processMessage(const std::string& msg_txt) override;

  private:
    /// Associate response callback
    MessageCallback associate_response_callback_;

    /// TTL Expired callback
    MessageCallback TTL_expired_callback_;

    /// To manage the Associate Session process
    SessionAssociation session_association_;

    /// To keep track of Associate Session timings
    AssociationTimings association_timings_;

    MessageChunk createEnvelope(const std::vector<std::string>& targets,
                                const std::string& message_type,
                                unsigned int timeout,
                                bool destination_report,
                                std::string& msg_id);

    std::string sendMessage(const std::vector<std::string>& targets,
                            const std::string& message_type,
                            unsigned int timeout,
                            bool destination_report,
                            const std::string& data_txt,
                            const std::vector<lth_jc::JsonContainer>& debug);

    // WebSocket Callback for the Connection instance to be triggered
    // on an onOpen event.
    void associateSession();

    // WebSocket Callback for the Connection instance to be triggered
    // on onClose or onFail events.
    void closeAssociationTimings();

    // PCP Callback executed by processMessage in case of an
    // associate session response.
    void associateResponseCallback(const ParsedChunks& parsed_chunks);

    // PCP Callback executed by processMessage when an error message
    // is received.
    void errorMessageCallback(const ParsedChunks& parsed_chunks);

    // PCP Callback executed by processMessage when a ttl_expired
    // message is received.
    void TTLMessageCallback(const ParsedChunks& parsed_chunks);
};

}  // namespace v1
}  // namespace PCPClient

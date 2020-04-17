#pragma once

#include <cpp-pcp-client/protocol/v2/message.hpp>

#include <cpp-pcp-client/connector/connector_base.hpp>

#include <memory>
#include <string>
#include <map>
#include <cstdint>

namespace PCPClient {
namespace v2 {

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
              uint32_t pong_timeouts_before_retry = 3,
              long ws_pong_timeout_ms = 5000);

    // legacy constructor: pre proxy
    Connector(std::vector<std::string> broker_ws_uris,
              std::string client_type,
              std::string ca_crt_path,
              std::string client_crt_path,
              std::string client_key_path,
              long ws_connection_timeout_ms = 5000,
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
              uint32_t pong_timeouts_before_retry = 3,
              long ws_pong_timeout_ms = 5000);

    // constructor for proxy addition
    Connector(std::vector<std::string> broker_ws_uris,
              std::string client_type,
              std::string ca_crt_path,
              std::string client_crt_path,
              std::string client_key_path,
              std::string client_crl_path,
              std::string ws_proxy,
              long ws_connection_timeout_ms = 5000,
              uint32_t pong_timeouts_before_retry = 3,
              long ws_pong_timeout_ms = 5000);

    /// Send the specified message.
    /// Throw a connection_processing_error in case of failure;
    /// throw a connection_not_init_error in case the connection
    /// has not been opened previously.
    void send(const Message& msg);

    /// send() overloads that create and send a message as specified.
    /// Return the ID of the message, as a string.
    ///
    /// The caller may specify:
    ///   - target: a PCP URI string
    ///   - message_type: schema name that identifies the message type
    ///   - data: as stringified JSON
    ///
    /// All methods:
    ///   - throw a connection_processing_error in case of failure;
    ///   - throw a connection_not_init_error in case the connection
    ///     has not been opened previously.
    std::string send(const std::string& target,
                     const std::string& message_type,
                     const lth_jc::JsonContainer& data_json,
                     const std::string& in_reply_to = "");

    std::string send(const std::string& target,
                     const std::string& message_type,
                     const std::string& data_txt,
                     const std::string& in_reply_to = "");

    std::string sendError(const std::string& target,
                          const std::string& in_reply_to,
                          const std::string& description);

  protected:
    // WebSocket Callback for the Connection instance to handle all
    // incoming messages.
    // Parse and validate the passed message; execute the callback
    // associated with the schema specified in the envelope.
    void processMessage(const std::string& msg_txt) override;

  private:
    // PCP Callback executed by processMessage when an error message
    // is received.
    void errorMessageCallback(const ParsedChunks&);
};

}  // namespace v2
}  // namespace PCPClient

#ifndef CTHUN_CLIENT_SRC_CONNECTOR_CONNECTOR_H_
#define CTHUN_CLIENT_SRC_CONNECTOR_CONNECTOR_H_

#include "./timings.h"

// See http://www.zaphoyd.com/websocketpp/manual/reference/cpp11-support
// for prepocessor directives and choosing between boost and cpp11
// C++11 STL tokens
#define _WEBSOCKETPP_CPP11_FUNCTIONAL_
#define _WEBSOCKETPP_CPP11_MEMORY_
#define _WEBSOCKETPP_CPP11_RANDOM_DEVICE_
#define _WEBSOCKETPP_CPP11_SYSTEM_ERROR_
#define _WEBSOCKETPP_CPP11_THREAD_

#include <websocketpp/common/connection_hdl.hpp>
#include <websocketpp/client.hpp>
// NB: we must include asio_client.hpp even if CTHUN_CLIENT_SECURE_TRANSPORT
// is not defined in order to define Context_Ptr
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>

#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <stdint.h>

namespace CthunClient {

// Constants

static const std::string PING_PAYLOAD_DEFAULT { "" };
static const uint32_t CONNECTION_BACKOFF_S { 2 };  // [s]
static const std::string DEFAULT_CLOSE_REASON { "Closed by client" };

// Configuration of the WebSocket transport layer

using Client_Type = websocketpp::client<websocketpp::config::asio_tls_client>;
using Context_Ptr = websocketpp::lib::shared_ptr<boost::asio::ssl::context>;
using Connection_Handle = websocketpp::connection_hdl;

// Frame opcodes

using Frame_Opcode = websocketpp::frame::opcode::value;
namespace Frame_Opcode_Values = websocketpp::frame::opcode;

// Connection States

namespace ConnectionStateValues {
    enum value_ {
        initialized = -1,
        connecting = 0,
        open = 1,
        closing = 2,
        closed = 3
    };
}  // namespace ConnectionStateValues

using ConnectionState = ConnectionStateValues::value_;

// Close Codes

namespace CloseCodeValues {
    enum value_ : uint16_t {
        normal = 1000,              // Normal connection closure
        abnormal_close = 1006,      // Abnormal
        subprotocol_error = 3000    // Generic subprotocol error
    };
}  // namespace CloseCodeValues

using CloseCode = CloseCodeValues::value_;

//
// Connector
//

class Connector {
  public:
    /// The Connector class provides the necessary logic to establish
    /// and use a Cthun connection over Websocket.
    /// The constructor throws an endpoint_error if it fails to
    /// configure the underlying WebSocket endpoint and the event
    /// handlers.
    Connector(const std::string& server_url,
              const std::string& ca_crt_path,
              const std::string& client_crt_path,
              const std::string& client_key_path);

    ~Connector();

    /// Set the callback that will be executed after the login message
    /// is sent
    void setPostLoginCallback(std::function<void()> postLogin_callback);

    /// Set the onMessage callback
    void setOnMessageCallback(
        std::function<void(std::string msg)> onMessage_callback);

    // Reset the onMessage and postLogin callbacks
    void resetCallbacks();

    /// Check the state of the WebSocket connection; in case it's not
    /// open, try to re-open it.
    /// Try to reopen for max_connect_attempts times or idefinetely,
    /// in case that parameter is 0 (as by default). This is done by
    /// following an exponential backoff.
    /// Throw a connection_error if it fails to open.
    void connect(int max_connect_attempts = 0);

    /// Send the message to the server.
    /// Throw a message_error in case of failure.
    void send(std::string msg);
    void send(void* const serialized_msg_ptr, size_t msg_len);

    /// Ping the server.
    /// Throw a message_error in case of failure.
    void ping(const std::string& binary_payload = PING_PAYLOAD_DEFAULT);

    /// Close the connection; reason and code are optional
    /// (respectively default to "Closed by client" and 'normal' as
    /// in rfc6455).
    /// Throw a message_error in case of failure.
    void close(CloseCode code = CloseCodeValues::normal,
               const std::string& reason = DEFAULT_CLOSE_REASON);

    /// Return the connection state
    ConnectionState getConnectionState();

    /// Periodically check the WebSocket connection state; re-open it
    /// in case it has dropped, otherwise ping the server
    // HERE(ale): we assume that the client will still be logged in
    // once the WebSocket connection is re-opened
    void monitorConnectionState();

  private:
    /// Cthun server url
    std::string server_url_;

    /// TLS certificates
    std::string ca_crt_path_;
    std::string client_crt_path_;
    std::string client_key_path_;

    /// Transport layer connection handle
    Connection_Handle connection_handle_;

    /// State of the connection (initialized, connectig, open,
    /// closing, or closed)
    std::atomic<ConnectionState> connection_state_;

    /// Consecutive pong timeouts counter (NB: useful for debug msgs)
    uint consecutive_pong_timeouts_ { 0 };

    /// Transport layer endpoint instance
    Client_Type endpoint_;

    /// Transport layer event loop thread
    std::shared_ptr<std::thread> endpoint_thread_;

    /// Callback function executed by the onOpen handler after sending
    /// the login message
    std::function<void()> post_login_callback_;

    /// Callback function executed by the onMessage handler
    std::function<void(std::string message)> on_message_callback_;

    /// Exponential backoff interval for re-connect
    uint32_t connection_backoff_s_ { CONNECTION_BACKOFF_S };

    /// Keep track of connection timings
    ConnectionTimings connection_timings_;

    /// Stop the event loop thread and perform the necessary clean up
    void cleanUp_();

    // Connect the endpoint
    void connect_();

    /// Event handlers
    Context_Ptr onTlsInit(Connection_Handle hdl);
    void onClose(Connection_Handle hdl);
    void onFail(Connection_Handle hdl);
    bool onPing(Connection_Handle hdl, std::string binary_payload);
    void onPong(Connection_Handle hdl, std::string binary_payload);
    void onPongTimeout(Connection_Handle hdl, std::string binary_payload);
    void onPreTCPInit(Connection_Handle hdl);
    void onPostTCPInit(Connection_Handle hdl);

    /// Handler executed by the transport layer in case of a
    /// WebSocket onOpen event. Calls onOpen_callback_(); in case it
    /// fails, the exception is filtered and the connection is closed.
    void onOpen(Connection_Handle hdl);

    /// Handler executed by the transport layer in case of a
    /// WebSocket onMessage event. Calls onMessage_callback_();
    /// in case it fails, the exception is filtered and logged.
    void onMessage(Connection_Handle hdl, Client_Type::message_ptr msg);
};

}  // namespace CthunClient

#endif  // CTHUN_CLIENT_SRC_CONNECTOR_CONNECTOR_H_

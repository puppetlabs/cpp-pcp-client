#ifndef CTHUN_CLIENT_SRC_CONNECTOR_CONNECTION_H_
#define CTHUN_CLIENT_SRC_CONNECTOR_CONNECTION_H_

#include "./timings.h"
#include "./client_metadata.h"

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
// is not defined in order to define WS_Context_Ptr
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

using WS_Client_Type = websocketpp::client<websocketpp::config::asio_tls_client>;
using WS_Context_Ptr = websocketpp::lib::shared_ptr<boost::asio::ssl::context>;
using WS_Connection_Handle = websocketpp::connection_hdl;

// Frame opcodes

using WS_Frame_Opcode = websocketpp::frame::opcode::value;
namespace WS_Frame_Opcode_Values = websocketpp::frame::opcode;

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
// Connection
//

class Connection {
  public:
    /// The Connection class provides the necessary logic to establish
    /// and use a Cthun connection over Websocket.
    /// The constructor throws an connection_config_error if it fails
    /// to configure the underlying WebSocket endpoint and the event
    /// handlers.
    Connection(const std::string& server_url,
               const ClientMetadata& client_metadata);

    ~Connection();

    /// Return the connection state
    ConnectionState getConnectionState();

    /// Set the onOpen callback.
    void setOnOpenCallback(std::function<void()> onOpen_callback);

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
    /// Throw a connection_processing_error in case of error during a
    /// connection attempt.
    /// Throw a connection_fatal_error in case in case it doesn't
    /// succeed after max_connect_attempts attempts.
    void connect(int max_connect_attempts = 0);

    /// Send a message to the server.
    /// Throw a connection_processing_error in case of failure while
    /// sending.
    void send(const std::string& msg);
    void send(void* const serialized_msg_ptr, size_t msg_len);

    /// Ping the server.
    /// Throw a connection_processing_error in case of failure.
    void ping(const std::string& binary_payload = PING_PAYLOAD_DEFAULT);

    /// Close the connection; reason and code are optional
    /// (respectively default to "Closed by client" and 'normal' as
    /// in rfc6455).
    /// Throw a connection_processing_error in case of failure.
    void close(CloseCode code = CloseCodeValues::normal,
               const std::string& reason = DEFAULT_CLOSE_REASON);

  private:
    /// Cthun server url
    std::string server_url_;

    /// Client metadata
    ClientMetadata client_metadata_;

    /// Transport layer connection handle
    WS_Connection_Handle connection_handle_;

    /// State of the connection (initialized, connectig, open,
    /// closing, or closed)
    std::atomic<ConnectionState> connection_state_;

    /// Consecutive pong timeouts counter (NB: useful for debug msgs)
    uint consecutive_pong_timeouts_ { 0 };

    /// Transport layer endpoint instance
    WS_Client_Type endpoint_;

    /// Transport layer event loop thread
    std::shared_ptr<std::thread> endpoint_thread_;

    // Callback function called by the onOpen handler.
    std::function<void()> onOpen_callback;

    /// Callback function executed by the onMessage handler
    std::function<void(const std::string& message)> onMessage_callback_;

    /// Exponential backoff interval for re-connect
    uint32_t connection_backoff_s_ { CONNECTION_BACKOFF_S };

    /// Keep track of connection timings
    ConnectionTimings connection_timings_;

    /// Stop the event loop thread and perform the necessary clean up
    void cleanUp_();

    // Connect the endpoint
    void connect_();

    /// Event handlers
    WS_Context_Ptr onTlsInit(WS_Connection_Handle hdl);
    void onClose(WS_Connection_Handle hdl);
    void onFail(WS_Connection_Handle hdl);
    bool onPing(WS_Connection_Handle hdl, std::string binary_payload);
    void onPong(WS_Connection_Handle hdl, std::string binary_payload);
    void onPongTimeout(WS_Connection_Handle hdl, std::string binary_payload);
    void onPreTCPInit(WS_Connection_Handle hdl);
    void onPostTCPInit(WS_Connection_Handle hdl);

    /// Handler executed by the transport layer in case of a
    /// WebSocket onOpen event. Calls onOpen_callback_(); in case it
    /// fails, the exception is filtered and the connection is closed.
    void onOpen(WS_Connection_Handle hdl);

    /// Handler executed by the transport layer in case of a
    /// WebSocket onMessage event. Calls onMessage_callback_();
    /// in case it fails, the exception is filtered and logged.
    void onMessage(WS_Connection_Handle hdl, WS_Client_Type::message_ptr msg);
};

}  // namespace CthunClient

#endif  // CTHUN_CLIENT_SRC_CONNECTOR_CONNECTION_H_

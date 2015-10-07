#ifndef CPP_PCP_CLIENT_SRC_CONNECTOR_CONNECTION_H_
#define CPP_PCP_CLIENT_SRC_CONNECTOR_CONNECTION_H_

#include <cpp-pcp-client/connector/timings.hpp>
#include <cpp-pcp-client/connector/client_metadata.hpp>

#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <stdint.h>

// Forward declarations for boost::asio
namespace boost {
    namespace asio {
        namespace ssl {
            class context;
        }
    }
}  // namespace boost

// Forward declarations for websocketpp
namespace websocketpp {
    template <typename T>
    class client;

    namespace config {
        struct asio_tls_client;
    }

    namespace message_buffer {
        namespace alloc {
            template <typename message>
            class con_msg_manager;
        }

        template <template<class> class con_msg_manager>
        class message;
    }

    namespace lib{
        using std::shared_ptr;
    }

    using connection_hdl = std::weak_ptr<void>;
}  // namespace websocket

namespace PCPClient {

// Constants

static const std::string PING_PAYLOAD_DEFAULT { "" };
static const uint32_t CONNECTION_BACKOFF_MS { 2000 };  // [ms]
static const std::string DEFAULT_CLOSE_REASON { "Closed by client" };

// Configuration of the WebSocket transport layer

using WS_Client_Type = websocketpp::client<websocketpp::config::asio_tls_client>;
using WS_Context_Ptr = websocketpp::lib::shared_ptr<boost::asio::ssl::context>;
using WS_Connection_Handle = websocketpp::connection_hdl;

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
    /// and use a PCP connection over Websocket.
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
    /// PCP server url
    std::string server_url_;

    /// Client metadata
    ClientMetadata client_metadata_;

    /// Transport layer connection handle
    WS_Connection_Handle connection_handle_;

    /// State of the connection (initialized, connectig, open,
    /// closing, or closed)
    std::atomic<ConnectionState> connection_state_;

    /// Consecutive pong timeouts counter (NB: useful for debug msgs)
    uint32_t consecutive_pong_timeouts_ { 0 };

    /// Transport layer endpoint instance
    std::unique_ptr<WS_Client_Type> endpoint_;

    /// Transport layer event loop thread
    std::shared_ptr<std::thread> endpoint_thread_;

    // Callback function called by the onOpen handler.
    std::function<void()> onOpen_callback;

    /// Callback function executed by the onMessage handler
    std::function<void(const std::string& message)> onMessage_callback_;

    /// Exponential backoff interval for re-connect
    uint32_t connection_backoff_ms_ { CONNECTION_BACKOFF_MS };

    /// Keep track of connection timings
    ConnectionTimings connection_timings_;

    /// Stop the event loop thread and perform the necessary clean up
    void cleanUp();

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
    void onMessage(
        WS_Connection_Handle hdl,
        std::shared_ptr<
            class websocketpp::message_buffer::message<
                websocketpp::message_buffer::alloc::con_msg_manager>> msg);
};

}  // namespace PCPClient

#endif  // CPP_PCP_CLIENT_SRC_CONNECTOR_CONNECTION_H_

#ifndef CPP_PCP_CLIENT_SRC_CONNECTOR_CONNECTION_H_
#define CPP_PCP_CLIENT_SRC_CONNECTOR_CONNECTION_H_

#include <cpp-pcp-client/connector/timings.hpp>
#include <cpp-pcp-client/connector/client_metadata.hpp>
#include <cpp-pcp-client/util/thread.hpp>
#include <cpp-pcp-client/export.h>

#include <string>
#include <vector>
#include <memory>
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

enum class ConnectionState {
    initialized = -1,
    connecting = 0,
    open = 1,
    closing = 2,
    closed = 3
};

// Close Codes
// Implemented as a plain enum so they cast to Websocketpp's close::status::value

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

class LIBCPP_PCP_CLIENT_EXPORT Connection {
  public:
    /// To keep track of WebSocket timings
    ConnectionTimings timings;

    /// The Connection class provides the necessary logic to establish
    /// and use a PCP connection over WebSocket.
    /// The constructor throws an connection_config_error if it fails
    /// to configure the underlying WebSocket endpoint and the event
    /// handlers.
    Connection(std::string broker_ws_uri,
               ClientMetadata client_metadata);

    Connection(std::vector<std::string> broker_ws_uris,
               ClientMetadata client_metadata);

    ~Connection();

    /// Return the connection state
    ConnectionState getConnectionState() const;

    /// Callback setters (callbacks will be executed by the
    /// WebSocket event handlers).
    void setOnOpenCallback(std::function<void()> onOpen_callback);
    void setOnMessageCallback(std::function<void(const std::string& msg)> onMessage_callback);
    void setOnCloseCallback(std::function<void()> onClose_callback);
    void setOnFailCallback(std::function<void()> onFail_callback);

    /// Reset all the callbacks
    void resetCallbacks();

    /// Check the state of the WebSocket connection; in case it's not
    /// open, try to re-open it.
    /// Try to reopen for max_connect_attempts times or indefinitely,
    /// in case that parameter is 0 (as by default). This is done by
    /// following an exponential backoff.
    /// Throw a connection_processing_error in case of error during a
    /// connection attempt.
    /// Throw a connection_fatal_error in case in case it doesn't
    /// succeed after max_connect_attempts attempts.
    void connect(int max_connect_attempts = 0);

    /// Send a message to the broker.
    /// Throw a connection_processing_error in case of failure while
    /// sending.
    void send(const std::string& msg);
    void send(void* const serialized_msg_ptr, size_t msg_len);

    /// Ping the broker.
    /// Throw a connection_processing_error in case of failure.
    void ping(const std::string& binary_payload = PING_PAYLOAD_DEFAULT);

    /// Close the connection; reason and code are optional
    /// (respectively default to "Closed by client" and 'normal' as
    /// in rfc6455).
    /// Throw a connection_processing_error in case of failure.
    void close(CloseCode code = CloseCodeValues::normal,
               const std::string& reason = DEFAULT_CLOSE_REASON);

  private:
    /// WebSocket URIs of PCP brokers; first entry is the default
    std::vector<std::string> broker_ws_uris_;

    /// Client metadata
    ClientMetadata client_metadata_;

    /// Transport layer connection handle
    WS_Connection_Handle connection_handle_;

    /// State of the connection (initialized, connecting, open,
    /// closing, or closed)
    std::atomic<ConnectionState> connection_state_;

    /// Tracks the broker we should be trying to connect to.
    /// Defaults to 0, and increments when a successful
    /// connection and the first reconnect attempt against that
    /// connection fail, or when an attempt to connect to a new
    /// broker fails. This value modulo size of the broker list
    /// identifies the current broker to target.
    std::atomic<size_t> connection_target_index_;

    /// Consecutive pong timeouts counter (NB: useful for debug msgs)
    uint32_t consecutive_pong_timeouts_ { 0 };

    /// Transport layer endpoint instance
    std::unique_ptr<WS_Client_Type> endpoint_;

    /// Transport layer event loop thread
    std::shared_ptr<Util::thread> endpoint_thread_;

    /// To synchronize the onOpen event
    Util::condition_variable onOpen_cv;
    Util::mutex onOpen_mtx;

    // Callback functions called by the WebSocket event handlers.
    std::function<void()> onOpen_callback_;
    std::function<void(const std::string& message)> onMessage_callback_;
    std::function<void()> onClose_callback_;
    std::function<void()> onFail_callback_;

    /// Exponential backoff interval for re-connect
    uint32_t connection_backoff_ms_ { CONNECTION_BACKOFF_MS };

    /// To manage the connection state
    Util::mutex state_mutex_;

    /// Connect and wait until the connection is open or for the
    /// configured connection_timeout
    void connectAndWait();

    /// Try closing the connection
    void tryClose();

    /// Stop the event loop thread and perform the necessary clean up
    void cleanUp();

    // Connect the endpoint
    void connect_();

    /// Return the current broker WebSocket URI to target
    std::string const& getWsUri();

    /// Switch broker WebSocket URI targets
    void switchWsUri();

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

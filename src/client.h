#ifndef CTHUN_CLIENT_SRC_CLIENT_H_
#define CTHUN_CLIENT_SRC_CLIENT_H_

// We need this hack to avoid the compilation error described in
// https://github.com/zaphoyd/websocketpp/issues/365
#define _WEBSOCKETPP_NULLPTR_TOKEN_ 0

// TODO(ale): check if we really need all C++11 macros

// See http://www.zaphoyd.com/websocketpp/manual/reference/cpp11-support
// for prepocessor directives to choose between boost and cpp11
// C++11 STL tokens
#define _WEBSOCKETPP_CPP11_FUNCTIONAL_
#define _WEBSOCKETPP_CPP11_MEMORY_
#define _WEBSOCKETPP_CPP11_RANDOM_DEVICE_
#define _WEBSOCKETPP_CPP11_SYSTEM_ERROR_
#define _WEBSOCKETPP_CPP11_THREAD_

#include "connection_metadata.h"

#include <websocketpp/common/connection_hdl.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <thread>

namespace Cthun {
namespace Client {

//
// Tokens
//

// TODO(ale): use a meaningful reason
static const std::string DEFAULT_CLOSE_REASON { "Closed by client" };

static const int DEFAULT_NUM_SEND_INTERVALS { 5 };
static const int DEFAULT_SEND_INTERVAL { 1000 };  // [ms]

//
// Aliases
//

using Client_Configuration
      = websocketpp::client<websocketpp::config::asio_tls_client>;

using Context_Ptr = websocketpp::lib::shared_ptr<boost::asio::ssl::context>;

using Message = std::string;

using Connection_Handle = websocketpp::connection_hdl;

using Close_Code = websocketpp::close::status::value;
namespace Close_Code_Values = websocketpp::close::status;

using Connection_State = websocketpp::session::state::value;
namespace Connection_State_Values = websocketpp::session::state;

//
// Errors
//

/// Base error class.
class client_error : public std::runtime_error {
  public:
    explicit client_error(std::string const& msg) : std::runtime_error(msg) {}
};

/// Connection error class.
class connection_error : public client_error {
  public:
    explicit connection_error(std::string const& msg) : client_error(msg) {}
};

/// Message sending error class.
class message_error : public client_error {
  public:
    explicit message_error(std::string const& msg) : client_error(msg) {}
};

/// Unknown connection id class.
class unknown_connection : public client_error {
  public:
    unknown_connection() : client_error(std_msg_) {}

    explicit unknown_connection(std::string const& msg)
        : client_error(std_msg_ + " " + msg) {}

  private:
    std::string std_msg_ { "Unknown connection" };
};

//
// Base Client
//

class BaseClient {
  public:
    BaseClient();

    /// Connect to the specified server and return the connection
    /// Handle.
    /// Throw a connection_error in case of failure.
    Connection_Handle connect(std::string url);

    /// Get the state of the specified connection as a
    /// Connection_State value (as in rfc6455).
    /// Throw an unknow_connection in case connection_id is unknown.
    Connection_State getStateOf(Connection_Handle hdl);

    /// Send message on the specified connection.
    /// Throw a message_error in case of failure.
    /// Throw an unknow_connection in case the specified connection is
    /// unknown.
    void send(Connection_Handle hdl, std::string msg);

    /// Send message on the specified connection. If the connection is
    /// in the CONNECTING state, waits for its establishment (i.e.
    /// CONNECTED state).
    /// This is done by periodically polling the state during a
    /// specified time interval, after which a message_error is
    /// thrown. The poll period is interval ms; the total time is
    /// (num_intervals * interval) ms; default values are 5 and
    /// 1000 ms respectively.
    /// Throw a message_error in case the connection is in CLOSING or
    /// CLOSED state; more in general, if the sending process fails.
    /// Throw an unknow_connection in case the specified connection is
    /// unknown.
    void sendWhenEstablished(Connection_Handle hdl, std::string msg,
                             int num_intervals = DEFAULT_NUM_SEND_INTERVALS,
                             int interval = DEFAULT_SEND_INTERVAL);

    /// Close the specified connection; reason and code are optional
    /// (respectively default to empty string and normal as rfc6455).
    /// Throw a connection_error in case of failure.
    /// Throw an unknow_connection in case the specified connection is
    /// unknown.
    void closeConnection(Connection_Handle hdl,
                         Close_Code code = Close_Code_Values::normal,
                         Message reason = DEFAULT_CLOSE_REASON);

    /// Close all connections.
    /// Throw a close_error in case of failure.
    void closeAllConnections();

    /// Event loop open callback.
    virtual void onOpen_(Connection_Handle hdl);

    /// Event loop close callback.
    virtual void onClose_(Connection_Handle hdl);

    /// Event loop failure callback.
    virtual void onFail_(Connection_Handle hdl);

    /// Event loop TLS init callback.
    virtual Context_Ptr onTlsInit_(Connection_Handle hdl);

    /// Event loop message callback.
    virtual void onMessage_(Connection_Handle hdl,
                            Client_Configuration::message_ptr msg);

  protected:
    Client_Configuration client_;
    // NB: connection handles are thread safe

    std::map<Connection_Handle, ConnectionMetadata,
             std::owner_less<Connection_Handle>> metadata_of_connections_;

    std::shared_ptr<std::thread> connection_thread_;

    /// Throw a unknown_connection if the connection is unknown.
    void checkConnectionHandle(Connection_Handle hdl);

    /// Gracefully terminates the client.
    /// This should be called by the subclass destructor.
    void shutdown();
};

}  // namespace Client
}  // namespace Cthun

#endif  // CTHUN_CLIENT_SRC_CLIENT_H_

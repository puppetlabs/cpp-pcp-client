#ifndef CTHUN_CLIENT_SRC_CLIENT_H_
#define CTHUN_CLIENT_SRC_CLIENT_H_

// We need this hack to avoid the compilation error described in
// https://github.com/zaphoyd/websocketpp/issues/365
#define _WEBSOCKETPP_NULLPTR_TOKEN_ 0

#include "common/uuid.h"

#include <websocketpp/common/connection_hdl.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>

// TODO(ale): use <websocketpp/config/asio_client.hpp> for TLS

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

// See http://www.zaphoyd.com/websocketpp/manual/reference/cpp11-support
// for prepocessor directives to choose between boost and cpp11
#include <thread>
// #include <websocketpp/common/thread.hpp>

namespace Cthun {
namespace Client {

//
// Aliases
//

typedef websocketpp::client<websocketpp::config::asio_client> Client_Configuration;

typedef std::string Message;

typedef Common::UUID Connection_ID;
static const auto getNewConnectionID = Common::getUUID;

typedef std::unordered_map<Connection_ID, websocketpp::connection_hdl>
    Connection_Handlers;
typedef std::pair<Connection_ID &, websocketpp::connection_hdl &>
    Connection_Handler_Pair;

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
    explicit unknown_connection(std::string const& msg)
        : client_error("Unknown connection ID " + msg) {}
};

//
// Test Client
//

class TestClient {
  public:
    TestClient() = delete;
    explicit TestClient(std::vector<Message> & messages);
    ~TestClient();

    /// Connect to the specified server and return a connection id.
    /// Raise a connection_error in case of failure.
    Connection_ID connect(std::string url);

    /// Get the state of the specified connection.
    /// Raise an unknow_connection in case connection_id is unknown.
    websocketpp::session::state::value getStateOf(Connection_ID connection_id);

    /// Send message on the specified connection.
    /// Raise a message_error in case of failure.
    /// Raise an unknow_connection in case connection_id is unknown.
    void send(Connection_ID connection_id, std::string msg);

    /// Close the specified connection.
    /// Raise a connection_error in case of failure.
    /// Raise an unknow_connection in case connection_id is unknown.
    void closeConnection(Connection_ID connection_id);

    /// Close all connections.
    /// Raise a close_error in case of failure.
    void closeAllConnections();

    /// Event loop open callback.
    void onOpen_(websocketpp::connection_hdl hdl);

    /// Event loop close callback.
    void onClose_(websocketpp::connection_hdl hdl);

    /// Event loop failure callback.
    void onFail_(websocketpp::connection_hdl hdl);

    /// Event loop failure callback.
    void onMessage_(websocketpp::connection_hdl hdl,
                    Client_Configuration::message_ptr msg);

  private:
    Client_Configuration client_;
    std::vector<Message> & messages_;
    // NB: connection handlers are thread safe
    Connection_Handlers connection_handlers_;
    std::shared_ptr<std::thread> connection_thread_;

    /// Return the connection handler given its id.
    /// Raise a unknown_connection in case the id is unknown.
    websocketpp::connection_hdl getConnectionHandler(Connection_ID connection_id);
};

}  // namespace Client
}  // namespace Cthun

#endif  // CTHUN_CLIENT_SRC_CLIENT_H_

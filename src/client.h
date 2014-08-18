#ifndef CTHUN_TEST_CLIENT_SRC_CLIENT_H_
#define CTHUN_TEST_CLIENT_SRC_CLIENT_H_

// We need this hack to avoid the compilation error described in
// https://github.com/zaphoyd/websocketpp/issues/365
#define _WEBSOCKETPP_NULLPTR_TOKEN_ 0

#include <websocketpp/common/connection_hdl.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>

// TODO(ale): use <websocketpp/config/asio_client.hpp> for TLS

#include <string>
#include <vector>

// See http://www.zaphoyd.com/websocketpp/manual/reference/cpp11-support
// for prepocessor directives to choose between boost and cpp11
#include <thread>
// #include <websocketpp/common/thread.hpp>


namespace Cthun_Test_Client {

//
// Errors
//

/// Connection error class.
class connection_error : public std::runtime_error {
  public:
    explicit connection_error(std::string const& msg) : std::runtime_error(msg) {}
};

/// Message sending error class.
class message_error : public std::runtime_error {
  public:
    explicit message_error(std::string const& msg) : std::runtime_error(msg) {}
};

//
// Client
//

typedef websocketpp::client<websocketpp::config::asio_client> client;

class Client {
  public:
    Client();
    ~Client();

    /// Connect to the specified server and return a connection handler
    /// Raise a connection_error in case of failure
    websocketpp::connection_hdl connect(std::string url);

    /// Send message on the specified connection
    /// Raise a message_error in case of failure
    void send(websocketpp::connection_hdl hdl, std::string message);

    /// Event loop open callback
    void on_open(websocketpp::connection_hdl hdl);

    /// Event loop close callback
    void on_close(websocketpp::connection_hdl hdl);

    /// Event loop failure callback
    void on_fail(websocketpp::connection_hdl hdl);

    // TODO(ale): clean this hack used to test client::run event loop
    void join_the_thread();

  private:
    client client_;
    std::thread the_thread_;
};

}  // namespace Cthun_Test_Client

#endif  // CTHUN_TEST_CLIENT_SRC_CLIENT_H_

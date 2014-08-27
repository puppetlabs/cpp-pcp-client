#include "client.h"

#include <iostream>

// NB: (from websocketpp wiki) "Per the websocket spec, a client can
// only have one connection in the connecting state at a time."

namespace Cthun {
namespace Client {

TestClient::TestClient(std::vector<Message> & messages)
    : messages_ { messages } {
    // Create and start endpoint
    client_.init_asio();
    client_.start_perpetual();

    // TODO(ale): setup logging and get rid of "###" stdout messages

    using websocketpp::lib::placeholders::_1;
    using websocketpp::lib::bind;
    client_.set_tls_init_handler(bind(&TestClient::onTlsInit_, this, ::_1));
    client_.set_open_handler(bind(&TestClient::onOpen_, this, ::_1));
    client_.set_close_handler(bind(&TestClient::onClose_, this, ::_1));
    client_.set_fail_handler(bind(&TestClient::onFail_, this, ::_1));
    client_.set_message_handler(bind(&TestClient::onMessage_, this, ::_1, ::_2));

    connection_thread_.reset(new std::thread(&Client_Configuration::run, &client_));
}

TestClient::~TestClient() {
    client_.stop_perpetual();

    TestClient::closeAllConnections();

    if (connection_thread_ != nullptr && connection_thread_->joinable()) {
        connection_thread_->join();
    }
}

Connection_ID TestClient::connect(std::string url) {
    // TODO(ale): url validation
    // TODO(ale): use the exception-based call
    websocketpp::lib::error_code ec;
    Client_Configuration::connection_ptr connection_p {
        client_.get_connection(url, ec) };

    if (ec) {
        throw connection_error { "Failed to connect to " + url + ": "
                                 + ec.message() };
    }

    websocketpp::connection_hdl hdl { connection_p->get_handle() };
    client_.connect(connection_p);

    Connection_ID id = getNewConnectionID();
    connection_handlers_.insert(Connection_Handler_Pair { id, hdl });
    return id;
}

// TODO(ale): is it safe to upgrade to pointer and get state?
// In alternative, keep the state inside connection_metadata...

websocketpp::session::state::value TestClient::getStateOf(
        Connection_ID connection_id) {
    websocketpp::connection_hdl hdl = getConnectionHandler(connection_id);
    // Upgrade to the pointer
    Client_Configuration::connection_ptr con = client_.get_con_from_hdl(hdl);
    return con->get_state();
}

// TODO(ale): do we need a message class?; handle binary payload

void TestClient::send(Connection_ID connection_id, std::string message) {
    websocketpp::connection_hdl hdl = getConnectionHandler(connection_id);
    websocketpp::lib::error_code ec;
    client_.send(hdl, message, websocketpp::frame::opcode::text, ec);
    if (ec) {
        throw message_error { "Failed to send message: " + ec.message() };
    }
}

void TestClient::closeConnection(Connection_ID connection_id) {
    websocketpp::connection_hdl hdl = getConnectionHandler(connection_id);
    websocketpp::lib::error_code ec;
    std::cout << "### About to close connection " << connection_id << std::endl;
    client_.close(hdl, websocketpp::close::status::normal, "", ec);
    if (ec) {
        throw message_error { "Failed to close connetion " + connection_id
                              + ": " + ec.message() };
    }
}

void TestClient::closeAllConnections() {
    for (auto handler_iter = connection_handlers_.begin();
        handler_iter != connection_handlers_.end(); handler_iter++) {
        closeConnection(handler_iter->first);
    }
    connection_handlers_ = Connection_Handlers {};
}

//
// Event loop callbacks
//

void TestClient::onOpen_(websocketpp::connection_hdl hdl) {
    std::cout << "### Triggered onOpen_()\n";

    // TODO(ale): use client.get_alog().write() as in telemetry_client

    for (auto msg : messages_) {
        // Send a message once the connection is open
        websocketpp::lib::error_code ec;

        // NB: as for WebSocket API, send() will give an error in case
        // the connection state in not OPEN (or CLOSING).

        // NB: send() will put msg on a queue... The only call that
        // provides flow control info is connection::get_buffered_amount()
        // (i.e., it could be used to throttle the message tx rate).

        client_.send(hdl, msg, websocketpp::frame::opcode::text, ec);
        if (ec) {
            throw message_error { "Failed to send message: " + ec.message() };
        }
        std::cout << "### Message sent (ASYNCHRONOUS - onOpen_) " << msg << "\n";
    }
}

void TestClient::onClose_(websocketpp::connection_hdl hdl) {
    std::cout << "### Triggered onClose_()\n";
}

void TestClient::onFail_(websocketpp::connection_hdl hdl) {
    std::cout << "### Triggered onFail_()\n";
}

Context_Ptr TestClient::onTlsInit_(websocketpp::connection_hdl hdl) {
    Context_Ptr ctx {
        new boost::asio::ssl::context(boost::asio::ssl::context::tlsv1) };

    try {
        ctx->set_options(boost::asio::ssl::context::default_workarounds |
                         boost::asio::ssl::context::no_sslv2 |
                         boost::asio::ssl::context::single_dh_use);
    } catch (std::exception& e) {
        std::cout << "### ERROR (tls): " << e.what() << std::endl;
    }
    return ctx;
}

void TestClient::onMessage_(websocketpp::connection_hdl hdl,
                            Client_Configuration::message_ptr msg) {
        std::cout << "### Got a MESSAGE!!! " << msg->get_payload() << std::endl;
}

//
// TestClient private interface
//

websocketpp::connection_hdl TestClient::getConnectionHandler(
        Connection_ID connection_id) {
    Connection_Handlers::iterator handler_iter =
        connection_handlers_.find(connection_id);
    if (handler_iter == connection_handlers_.end()) {
        throw unknown_connection { connection_id };
    }
    return handler_iter->second;
}

}  // namespace Client
}  // namespace Cthun

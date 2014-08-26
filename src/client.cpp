#include "client.h"
#include <iostream>

// NB: (from websocketpp wiki) "Per the websocket spec, a client can
// only have one connection in the connecting state at a time."

namespace Cthun {
namespace Client {

TestClient::TestClient() {
    std::cout << "### Inside Client init!\n";

    // Create endpoint
    client_.init_asio();

    // TODO(ale): setup log (as in the telemetry_client example)

    using websocketpp::lib::placeholders::_1;
    using websocketpp::lib::bind;
    client_.set_open_handler(bind(&TestClient::onOpen_, this, ::_1));
    client_.set_close_handler(bind(&TestClient::onClose_, this, ::_1));
    client_.set_fail_handler(bind(&TestClient::onFail_, this, ::_1));
    client_.set_message_handler(bind(&TestClient::onMessage_, this, ::_1, ::_2));
}

TestClient::~TestClient() {
    TestClient::closeAllConnections();
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

    // TODO(ale): check state of the connection;
    // check if it's possible to wait until it's OPEN (or the logic
    // can only be event-based)

    Connection_ID id = getNewConnectionID();
    connection_threads_.insert(Connection_Thread_Pair {
        id, std::thread(&Client_Configuration::run, &client_) });
    connection_handlers_.insert(Connection_Handler_Pair { id, hdl });
    return id;
}

// TODO(ale): message class (?); handle binary payload

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
    client_.close(hdl, websocketpp::close::status::normal, "", ec);
    if (ec) {
        throw message_error { "Failed to close connetion " + connection_id
                              + ": " + ec.message() };
    }
    joinConnectionThread(connection_id);
}

void TestClient::closeAllConnections() {
    for (auto handler_iter = connection_handlers_.begin();
        handler_iter != connection_handlers_.end(); handler_iter++) {
        closeConnection(handler_iter->first);
    }
}

//
// Event loop callbacks
//

void TestClient::onOpen_(websocketpp::connection_hdl hdl) {
    std::cout << "### Triggered onOpen_()\n";
    std::string message { "### Message payload (asynchronous) ###" };

    // TODO(ale): use client.get_alog().write() as in telemetry_client

    // Send a message once the connection is open
    websocketpp::lib::error_code ec;
    client_.send(hdl, message, websocketpp::frame::opcode::text, ec);
    if (ec) {
        throw message_error { "Failed to send message: " + ec.message() };
    }

    std::cout << "### Message sent (ASYNCHRONOUS - onOpen_)\n";
}

void TestClient::onClose_(websocketpp::connection_hdl hdl) {
    std::cout << "### Triggered onClose_()\n";
}

void TestClient::onFail_(websocketpp::connection_hdl hdl) {
    std::cout << "### Triggered onFail_()\n";
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

void TestClient::joinConnectionThread(Connection_ID connection_id) {
    Connection_Threads::iterator thread_iter =
        connection_threads_.find(connection_id);
    if (thread_iter == connection_threads_.end()) {
        throw unknown_connection { connection_id };
    }

    thread_iter->second.join();
}

void TestClient::joinAllConnectionThreads() {
    for (auto thread_iter = connection_threads_.begin();
        thread_iter != connection_threads_.end(); thread_iter++) {
        thread_iter->second.join();
    }
}

}  // namespace Client
}  // namespace Cthun

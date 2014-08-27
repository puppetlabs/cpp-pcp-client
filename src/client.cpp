#include "client.h"

#include <iostream>

// NB: (from websocketpp wiki) "Per the websocket spec, a client can
// only have one connection in the connecting state at a time."

namespace Cthun {
namespace Client {

BaseClient::BaseClient() {
    // Create and start endpoint
    client_.init_asio();
    client_.start_perpetual();

    // TODO(ale): setup logging and get rid of "###" stdout messages

    using websocketpp::lib::placeholders::_1;
    using websocketpp::lib::bind;
    client_.set_tls_init_handler(bind(&BaseClient::onTlsInit_, this, ::_1));
    client_.set_open_handler(bind(&BaseClient::onOpen_, this, ::_1));
    client_.set_close_handler(bind(&BaseClient::onClose_, this, ::_1));
    client_.set_fail_handler(bind(&BaseClient::onFail_, this, ::_1));
    client_.set_message_handler(bind(&BaseClient::onMessage_, this, ::_1, ::_2));

    connection_thread_.reset(new std::thread(&Client_Configuration::run, &client_));
}

Connection_ID BaseClient::connect(std::string url) {
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

websocketpp::session::state::value BaseClient::getStateOf(
        Connection_ID connection_id) {
    websocketpp::connection_hdl hdl = getConnectionHandler(connection_id);
    // Upgrade to the pointer
    Client_Configuration::connection_ptr con = client_.get_con_from_hdl(hdl);
    return con->get_state();
}

// TODO(ale): do we need a message class?; handle binary payload

void BaseClient::send(Connection_ID connection_id, std::string message) {
    websocketpp::connection_hdl hdl = getConnectionHandler(connection_id);
    websocketpp::lib::error_code ec;
    client_.send(hdl, message, websocketpp::frame::opcode::text, ec);
    if (ec) {
        throw message_error { "Failed to send message: " + ec.message() };
    }
}

void BaseClient::closeConnection(Connection_ID connection_id) {
    websocketpp::connection_hdl hdl = getConnectionHandler(connection_id);
    websocketpp::lib::error_code ec;
    std::cout << "### About to close connection " << connection_id << std::endl;
    client_.close(hdl, websocketpp::close::status::normal, "", ec);
    if (ec) {
        throw message_error { "Failed to close connetion " + connection_id
                              + ": " + ec.message() };
    }
}

void BaseClient::closeAllConnections() {
    for (auto handler_iter = connection_handlers_.begin();
        handler_iter != connection_handlers_.end(); handler_iter++) {
        closeConnection(handler_iter->first);
    }
    connection_handlers_ = Connection_Handlers {};
}

//
// BaseClient private interface
//

websocketpp::connection_hdl BaseClient::getConnectionHandler(
        Connection_ID connection_id) {
    Connection_Handlers::iterator handler_iter =
        connection_handlers_.find(connection_id);
    if (handler_iter == connection_handlers_.end()) {
        throw unknown_connection { connection_id };
    }
    return handler_iter->second;
}

void BaseClient::shutdown() {
    client_.stop_perpetual();
    closeAllConnections();
    if (connection_thread_ != nullptr && connection_thread_->joinable()) {
        connection_thread_->join();
    }
}


}  // namespace Client
}  // namespace Cthun

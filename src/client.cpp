#include "client.h"

#include <iostream>
#include <chrono>

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

    connection_thread_.reset(
        new std::thread(&Client_Configuration::run, &client_));
}

Connection_Handle BaseClient::connect(std::string url) {
    // TODO(ale): url validation
    // TODO(ale): use the exception-based call
    websocketpp::lib::error_code ec;
    Client_Configuration::connection_ptr connection_p {
        client_.get_connection(url, ec) };

    if (ec) {
        throw connection_error { "Failed to connect to " + url + ": "
                                 + ec.message() };
    }

    ConnectionMetadata connection_metadata {};
    Connection_Handle hdl { connection_p->get_handle() };
    client_.connect(connection_p);
    metadata_of_connections_.insert(
        std::pair<Connection_Handle, ConnectionMetadata> {
            hdl, connection_metadata });
    return hdl;
}

// TODO(ale): is it safe to upgrade to pointer and get state?
// In alternative, keep the state inside connection_metadata...

Connection_State BaseClient::getStateOf(Connection_Handle hdl) {
    checkConnectionHandle(hdl);
    // Upgrade to the pointer
    Client_Configuration::connection_ptr con = client_.get_con_from_hdl(hdl);
    return con->get_state();
}

// TODO(ale): do we need a message class?; handle binary payload
// TODO(ale): use ConnectionMetadata state to validate

void BaseClient::send(Connection_Handle hdl, std::string message) {
    checkConnectionHandle(hdl);
    websocketpp::lib::error_code ec;
    client_.send(hdl, message, websocketpp::frame::opcode::text, ec);
    if (ec) {
        throw message_error { "Failed to send message: " + ec.message() };
    }
}

void BaseClient::sendWhenEstablished(Connection_Handle hdl, std::string msg,
                                     int num_intervals, int interval_duration) {
    Connection_State current_state { getStateOf(hdl) };

    if (current_state == Connection_State_Values::open) {
        send(hdl, msg);
    } else if (current_state != Connection_State_Values::connecting) {
        int interval_idx { 0 };
        while (interval_idx < num_intervals) {
            if (getStateOf(hdl) == Connection_State_Values::open) {
                send(hdl, msg);
                return;
            }
            interval_idx++;
            std::this_thread::sleep_for(
                std::chrono::milliseconds(interval_duration));
        }
        throw message_error { "Send timeout" };
    } else {
        // TODO(ale): identify connection
        throw message_error { "Invalid connection state" };
    }
}


void BaseClient::closeConnection(Connection_Handle hdl, Close_Code code,
                                 Message reason) {
    checkConnectionHandle(hdl);
    websocketpp::lib::error_code ec;
    // TODO(ale): connection id from ConnectionMetadata
    std::cout << "### About to close connection " << std::endl;
    client_.close(hdl, code, reason, ec);
    if (ec) {
        throw message_error { "Failed to close connetion: " + ec.message() };
    }
}

void BaseClient::closeAllConnections() {
    for (auto metadata_iter = metadata_of_connections_.begin();
         metadata_iter != metadata_of_connections_.end(); metadata_iter++) {
        closeConnection(metadata_iter->first);
    }
    metadata_of_connections_.clear();
}

//
// Event loop callbacks
//

// TODO(ale): log message at the right log level for all callbacks
// TODO(ale): callbacks should set the ConnectionMetadata state

void BaseClient::onOpen_(Connection_Handle hdl) {}

void BaseClient::onClose_(Connection_Handle hdl) {}

void BaseClient::onFail_(Connection_Handle hdl) {}

Context_Ptr BaseClient::onTlsInit_(Connection_Handle hdl) {
    Context_Ptr ctx {
        new boost::asio::ssl::context(boost::asio::ssl::context::tlsv1) };

    try {
        ctx->set_options(boost::asio::ssl::context::default_workarounds |
                         boost::asio::ssl::context::no_sslv2 |
                         boost::asio::ssl::context::single_dh_use);
    } catch (std::exception& e) {
        // TODO(ale): this is what the the debug_client does and it
        // seems to work, nonetheless check if we need to raise here
        std::cout << "### ERROR (tls): " << e.what() << std::endl;
    }
    return ctx;
}

void BaseClient::onMessage_(Connection_Handle hdl,
                            Client_Configuration::message_ptr msg) {
    std::cout << "### Got a MESSAGE!!! " << msg->get_payload() << std::endl;
}

//
// BaseClient private interface
//

void BaseClient::checkConnectionHandle(Connection_Handle hdl) {
    std::map<Connection_Handle, ConnectionMetadata,
             std::owner_less<Connection_Handle>>::iterator metadata_iter
                = metadata_of_connections_.find(hdl);
    if (metadata_iter == std::end(metadata_of_connections_)) {
        // TODO(ale): get some id from ConnectionMetadata
        throw unknown_connection {};
    }
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

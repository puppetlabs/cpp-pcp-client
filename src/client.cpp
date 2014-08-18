#include "client.h"
#include <iostream>

// NB: (from websocketpp wiki) "Per the websocket spec, a client can
// only have one connection in the connecting state at a time."

namespace Cthun_Test_Client {

Client::Client() {
    std::cout << "### Inside Client init!\n";

    // Create endpoint
    client_.init_asio();

    // TODO(ale): setup log (as in the telemetry_client example)

    // bind stuff (as in the telemetry_client example)
    using websocketpp::lib::placeholders::_1;
    using websocketpp::lib::bind;
    client_.set_open_handler(bind(&Client::on_open, this, ::_1));
    client_.set_close_handler(bind(&Client::on_close, this, ::_1));
    client_.set_fail_handler(bind(&Client::on_fail, this, ::_1));
}

Client::~Client() {
    Client::join_the_thread();
}

websocketpp::connection_hdl Client::connect(std::string url) {
    websocketpp::lib::error_code ec;
    client::connection_ptr con { client_.get_connection(url, ec) };
    if (ec) {
        throw connection_error { "Failed to connect to " + url + ": "
                                 + ec.message() };
    }

    websocketpp::connection_hdl hdl { con->get_handle() };
    client_.connect(con);

    // TODO(ale): clean this
    the_thread_ = std::thread { &client::run, &client_ };
    return hdl;
}

void Client::join_the_thread() {
    the_thread_.join();
}

void Client::send(websocketpp::connection_hdl hdl, std::string message) {
    websocketpp::lib::error_code ec;
    client_.send(hdl, message, websocketpp::frame::opcode::text, ec);
    if (ec) {
        throw message_error { "Failed to send message: " + ec.message() };
    }
}

//
// Event loop callbacks
//

void Client::on_open(websocketpp::connection_hdl hdl) {
    std::cout << "### Triggered on_open()\n";
    // TODO(ale): use client.get_alog().write() as in telemetry_client
}

void Client::on_close(websocketpp::connection_hdl hdl) {
    std::cout << "### Triggered on_close()\n";
}

void Client::on_fail(websocketpp::connection_hdl hdl) {
    std::cout << "### Triggered on_fail()\n";
}

}  // namespace Cthun_Test_Client

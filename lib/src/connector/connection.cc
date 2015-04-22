// See http://www.zaphoyd.com/websocketpp/manual/reference/cpp11-support
// for prepocessor directives and choosing between boost and cpp11
// C++11 STL tokens
#define _WEBSOCKETPP_CPP11_FUNCTIONAL_
#define _WEBSOCKETPP_CPP11_MEMORY_
#define _WEBSOCKETPP_CPP11_RANDOM_DEVICE_
#define _WEBSOCKETPP_CPP11_SYSTEM_ERROR_
#define _WEBSOCKETPP_CPP11_THREAD_

#include <cthun-client/connector/connection.hpp>
#include <cthun-client/connector/errors.hpp>
#include <cthun-client/data_container/data_container.hpp>
#include <cthun-client/protocol/message.hpp>

#include <websocketpp/common/connection_hdl.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>

#define LEATHERMAN_LOGGING_NAMESPACE CTHUN_CLIENT_LOGGING_PREFIX".connection"

#include <leatherman/logging/logging.hpp>

#include <chrono>
#include <cstdio>
#include <iostream>

// TODO(ale): disable assert() once we're confident with the code...
// To disable assert()
// #define NDEBUG
#include <cassert>

namespace CthunClient {

//
// Constants
//

static const uint CONNECTION_MIN_INTERVAL { 200000 };  // [us]
static const uint CONNECTION_BACKOFF_LIMIT { 33 };  // [s]
static const uint CONNECTION_BACKOFF_MULTIPLIER { 2 };

//
// Connection
//

Connection::Connection(const std::string& server_url,
                       const ClientMetadata& client_metadata)
        : server_url_ { server_url },
          client_metadata_ { client_metadata },
          connection_state_ { ConnectionStateValues::initialized },
          consecutive_pong_timeouts_ { 0 },
          endpoint_ { new WS_Client_Type() }{
    // Turn off websocketpp logging to avoid runtime errors (see CTH-69)
    endpoint_->clear_access_channels(websocketpp::log::alevel::all);
    endpoint_->clear_error_channels(websocketpp::log::elevel::all);

    // Initialize the transport system. Note that in perpetual mode,
    // the event loop does not terminate when there are no connections
    endpoint_->init_asio();
    endpoint_->start_perpetual();

    try {
        // Bind the event handlers
        using websocketpp::lib::bind;
        endpoint_->set_tls_init_handler(bind(&Connection::onTlsInit, this, ::_1));
        endpoint_->set_open_handler(bind(&Connection::onOpen, this, ::_1));
        endpoint_->set_close_handler(bind(&Connection::onClose, this, ::_1));
        endpoint_->set_fail_handler(bind(&Connection::onFail, this, ::_1));
        endpoint_->set_message_handler(bind(&Connection::onMessage, this, ::_1, ::_2));
        endpoint_->set_ping_handler(bind(&Connection::onPing, this, ::_1, ::_2));
        endpoint_->set_pong_handler(bind(&Connection::onPong, this, ::_1, ::_2));
        endpoint_->set_pong_timeout_handler(bind(&Connection::onPongTimeout, this, ::_1, ::_2));
        endpoint_->set_tcp_pre_init_handler(bind(&Connection::onPreTCPInit, this, ::_1));
        endpoint_->set_tcp_post_init_handler(bind(&Connection::onPostTCPInit, this, ::_1));

        // Start the event loop thread
        endpoint_thread_.reset(new std::thread(&WS_Client_Type::run, endpoint_.get()));
    } catch (...) {
        LOG_DEBUG("Failed to configure the websocket endpoint; about to stop "
                  "the event loop");
        cleanUp();
        throw connection_config_error { "failed to initialize" };
    }
}

Connection::~Connection() {
    cleanUp();
}

ConnectionState Connection::getConnectionState() {
    return connection_state_.load();
}

//
// Callback modifiers
//

void Connection::setOnOpenCallback(std::function<void()> c_b) {
    onOpen_callback = c_b;
}

void Connection::setOnMessageCallback(std::function<void(std::string msg)> c_b) {
    onMessage_callback_ = c_b;
}

void Connection::resetCallbacks() {
    onOpen_callback = [](){};  // NOLINT [false positive readability/braces]
    onMessage_callback_ = [](std::string message){};  // NOLINT [false positive readability/braces]
}

//
// Synchronous calls
//

void Connection::connect(int max_connect_attempts) {
    // FSM: states are ConnectionStateValues; as for the transitions,
    //      we assume that the connection_state_:
    // - can be set to 'initialized' only by the Connection constructor;
    // - is set to 'connecting' by connect_();
    // - after a connect_() call, it will become, eventually, open or
    //   closed.

    ConnectionState previous_c_s = connection_state_.load();
    ConnectionState current_c_s;
    int idx { 0 };
    bool try_again { true };
    bool got_max_backoff { false };

    do {
        current_c_s = connection_state_.load();
        idx++;
        if (max_connect_attempts) {
            try_again = (idx < max_connect_attempts);
        }
        got_max_backoff |= (connection_backoff_s_ * CONNECTION_BACKOFF_MULTIPLIER
                            >= CONNECTION_BACKOFF_LIMIT);

        switch (current_c_s) {
        case(ConnectionStateValues::initialized):
            assert(previous_c_s == ConnectionStateValues::initialized);
            connect_();
            usleep(CONNECTION_MIN_INTERVAL);
            break;

        case(ConnectionStateValues::connecting):
            previous_c_s = current_c_s;
            usleep(CONNECTION_MIN_INTERVAL);
            continue;

        case(ConnectionStateValues::open):
            if (previous_c_s != ConnectionStateValues::open) {
                LOG_INFO("Successfully established connection to Cthun server");
                connection_backoff_s_ = CONNECTION_BACKOFF_S;
            }
            return;

        case(ConnectionStateValues::closing):
            previous_c_s = current_c_s;
            usleep(CONNECTION_MIN_INTERVAL);
            continue;

        case(ConnectionStateValues::closed):
            assert(previous_c_s != ConnectionStateValues::open);
            if (previous_c_s == ConnectionStateValues::closed) {
                connect_();
                usleep(CONNECTION_MIN_INTERVAL);
                previous_c_s = ConnectionStateValues::connecting;
            } else {
                LOG_INFO("Failed to connect; retrying in %1% seconds",
                         connection_backoff_s_);
                sleep(connection_backoff_s_);
                connect_();
                usleep(CONNECTION_MIN_INTERVAL);
                if (try_again && !got_max_backoff) {
                    connection_backoff_s_ *= CONNECTION_BACKOFF_MULTIPLIER;
                }
            }
            break;
        }
    } while (try_again);

    connection_backoff_s_ = CONNECTION_BACKOFF_S;
    throw connection_fatal_error { "failed to connect after "
                                   + std::to_string(idx) + " attempt"
                                   + (idx > 1 ? "s" : "") };
}

void Connection::send(const std::string& msg) {
    websocketpp::lib::error_code ec;
    endpoint_->send(connection_handle_,
                   msg,
                   websocketpp::frame::opcode::binary,
                   ec);
    if (ec) {
        throw connection_processing_error { "failed to send message: "
                                            + ec.message() };
    }
}

void Connection::send(void* const serialized_msg_ptr, size_t msg_len) {
    websocketpp::lib::error_code ec;
    endpoint_->send(connection_handle_,
                   serialized_msg_ptr,
                   msg_len,
                   websocketpp::frame::opcode::binary,
                   ec);
    if (ec) {
        throw connection_processing_error { "failed to send message: "
                                            + ec.message() };
    }
}

void Connection::ping(const std::string& binary_payload) {
    websocketpp::lib::error_code ec;
    endpoint_->ping(connection_handle_, binary_payload, ec);
    if (ec) {
        throw connection_processing_error { "failed to ping: " + ec.message() };
    }
}

void Connection::close(CloseCode code, const std::string& reason) {
    LOG_DEBUG("About to close connection");
    websocketpp::lib::error_code ec;
    endpoint_->close(connection_handle_, code, reason, ec);
    if (ec) {
        throw connection_processing_error { "failed to close connetion: "
                                            + ec.message() };
    }
}

//
// Private interface
//

void Connection::cleanUp() {
    endpoint_->stop_perpetual();
    if (connection_state_ == ConnectionStateValues::open) {
        try {
            close();
        } catch (connection_processing_error& e) {
            LOG_ERROR("Failed to close the connection: %1%", e.what());
        }
    }
    if (endpoint_thread_ != nullptr && endpoint_thread_->joinable()) {
        endpoint_thread_->join();
    }
}

void Connection::connect_() {
    connection_state_ = ConnectionStateValues::connecting;
    connection_timings_ = ConnectionTimings();
    websocketpp::lib::error_code ec;
    WS_Client_Type::connection_ptr connection_ptr {
        endpoint_->get_connection(server_url_, ec) };
    if (ec) {
        throw connection_processing_error { "failed to connect to " + server_url_
                                            + ": " + ec.message() };
    }
    connection_handle_ = connection_ptr->get_handle();
    endpoint_->connect(connection_ptr);
}

//
// Event handlers (private)
//

WS_Context_Ptr Connection::onTlsInit(WS_Connection_Handle hdl) {
    LOG_TRACE("WebSocket TLS initialization event");
    // NB: for TLS certificates, refer to:
    // www.boost.org/doc/libs/1_56_0/doc/html/boost_asio/reference/ssl__context.html
    WS_Context_Ptr ctx {
        new boost::asio::ssl::context(boost::asio::ssl::context::tlsv1) };
    try {
        ctx->set_options(boost::asio::ssl::context::default_workarounds |
                         boost::asio::ssl::context::no_sslv2 |
                         boost::asio::ssl::context::single_dh_use);
        ctx->use_certificate_file(client_metadata_.crt,
                                  boost::asio::ssl::context::file_format::pem);
        ctx->use_private_key_file(client_metadata_.key,
                                  boost::asio::ssl::context::file_format::pem);
        ctx->load_verify_file(client_metadata_.ca);
    } catch (std::exception& e) {
        LOG_ERROR("Failed to configure TLS: %1%", e.what());
    }
    return ctx;
}

void Connection::onClose(WS_Connection_Handle hdl) {
    connection_timings_.close = std::chrono::high_resolution_clock::now();
    LOG_TRACE("WebSocket connection closed");
    connection_state_ = ConnectionStateValues::closed;
}

void Connection::onFail(WS_Connection_Handle hdl) {
    connection_timings_.close = std::chrono::high_resolution_clock::now();
    connection_timings_.connection_failed = true;
    LOG_DEBUG("WebSocket on fail event - %1%", connection_timings_.toString());
    connection_state_ = ConnectionStateValues::closed;
}

bool Connection::onPing(WS_Connection_Handle hdl, std::string binary_payload) {
    LOG_TRACE("WebSocket onPing event - payload: %1%", binary_payload);
    // Returning true so the transport layer will send back a pong
    return true;
}

void Connection::onPong(WS_Connection_Handle hdl, std::string binary_payload) {
    LOG_DEBUG("WebSocket onPong event");
    if (consecutive_pong_timeouts_) {
        consecutive_pong_timeouts_ = 0;
    }
}

void Connection::onPongTimeout(WS_Connection_Handle hdl,
                               std::string binary_payload) {
    LOG_WARNING("WebSocket onPongTimeout event (%1% consecutive)",
                consecutive_pong_timeouts_++);
}

void Connection::onPreTCPInit(WS_Connection_Handle hdl) {
    connection_timings_.tcp_pre_init = std::chrono::high_resolution_clock::now();
    LOG_TRACE("WebSocket pre-TCP initialization event");
}

void Connection::onPostTCPInit(WS_Connection_Handle hdl) {
    connection_timings_.tcp_post_init = std::chrono::high_resolution_clock::now();
    LOG_TRACE("WebSocket post-TCP initialization event");
}

void Connection::onOpen(WS_Connection_Handle hdl) {
    connection_timings_.open = std::chrono::high_resolution_clock::now();
    connection_timings_.connection_started = true;
    LOG_DEBUG("WebSocket on open event - %1%", connection_timings_.toString());
    LOG_INFO("Cthun connection established");
    connection_state_ = ConnectionStateValues::open;

    if (onOpen_callback) {
        try {
            onOpen_callback();
            return;
        } catch (std::exception&  e) {
            LOG_ERROR("onOpen callback failure: %1%; closing the "
                      "WebSocketconnection", e.what());
        } catch (...) {
            LOG_ERROR("onOpen callback failure; closing the WebSocket "
                      "connection");
        }

        close(CloseCodeValues::normal, "onOpen callback failure");
    }
}

void Connection::onMessage(WS_Connection_Handle hdl,
                           WS_Client_Type::message_ptr msg) {
    if (onMessage_callback_) {
        try {
            // NB: on_message_callback_ should not raise; in case of
            // failure; it must be able to notify back the error...
            onMessage_callback_(msg->get_payload());
        } catch (std::exception&  e) {
            LOG_ERROR("onMessage callback failure: %1%", e.what());
        } catch (...) {
            LOG_ERROR("onMessage callback failure: unexpected error");
        }
    }
}

}  // namespace CthunClient

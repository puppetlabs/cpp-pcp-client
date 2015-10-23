// See http://www.zaphoyd.com/websocketpp/manual/reference/cpp11-support
// for prepocessor directives and choosing between boost and cpp11
// C++11 STL tokens
#define _WEBSOCKETPP_CPP11_FUNCTIONAL_
#define _WEBSOCKETPP_CPP11_MEMORY_
#define _WEBSOCKETPP_CPP11_RANDOM_DEVICE_
#define _WEBSOCKETPP_CPP11_SYSTEM_ERROR_
#define _WEBSOCKETPP_NO_CPP11_THREAD_

#include <cpp-pcp-client/connector/connection.hpp>
#include <cpp-pcp-client/connector/errors.hpp>
#include <cpp-pcp-client/protocol/message.hpp>
#include <cpp-pcp-client/util/thread.hpp>
#include <cpp-pcp-client/util/chrono.hpp>

#include <websocketpp/common/connection_hdl.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>

#define LEATHERMAN_LOGGING_NAMESPACE CPP_PCP_CLIENT_LOGGING_PREFIX".connection"

#include <leatherman/logging/logging.hpp>

#include <cstdio>
#include <iostream>
#include <random>

// TODO(ale): disable assert() once we're confident with the code...
// To disable assert()
// #define NDEBUG
#include <cassert>

namespace PCPClient {

//
// Constants
//

static const uint32_t CONNECTION_MIN_INTERVAL { 200 };  // [ms]
static const uint32_t CONNECTION_BACKOFF_LIMIT { 33000 };  // [ms]
static const uint32_t CONNECTION_BACKOFF_MULTIPLIER { 2 };

//
// Connection
//

Connection::Connection(const std::string& broker_ws_uri,
                       const ClientMetadata& client_metadata)
        : broker_ws_uri_ { broker_ws_uri },
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
        endpoint_->set_tls_init_handler(
            std::bind(&Connection::onTlsInit, this, std::placeholders::_1));
        endpoint_->set_open_handler(
            std::bind(&Connection::onOpen, this, std::placeholders::_1));
        endpoint_->set_close_handler(
            std::bind(&Connection::onClose, this, std::placeholders::_1));
        endpoint_->set_fail_handler(
            std::bind(&Connection::onFail, this, std::placeholders::_1));
        endpoint_->set_message_handler(
            std::bind(&Connection::onMessage, this,
                      std::placeholders::_1, std::placeholders::_2));
        endpoint_->set_ping_handler(
            std::bind(&Connection::onPing, this,
                      std::placeholders::_1, std::placeholders::_2));
        endpoint_->set_pong_handler(
            std::bind(&Connection::onPong, this,
                      std::placeholders::_1, std::placeholders::_2));
        endpoint_->set_pong_timeout_handler(
            std::bind(&Connection::onPongTimeout, this,
                      std::placeholders::_1, std::placeholders::_2));
        endpoint_->set_tcp_pre_init_handler(
            std::bind(&Connection::onPreTCPInit, this, std::placeholders::_1));
        endpoint_->set_tcp_post_init_handler(
            std::bind(&Connection::onPostTCPInit, this, std::placeholders::_1));

        // Start the event loop thread
        endpoint_thread_.reset(new Util::thread(&WS_Client_Type::run, endpoint_.get()));
    } catch (...) {
        LOG_DEBUG("Failed to configure the WebSocket endpoint; about to stop "
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
    // FSM
    //  - states are ConnectionStateValues:
    //      * initialized - connecting - open - closing - closed
    //  - for the transitions, we assume that the connection_state_:
    //      * can be set to 'initialized' only by the Connection constructor;
    //      * is set to 'connecting' by connect_();
    //      * after a connect_() call, it will become, eventually, open or
    //        closed.

    ConnectionState previous_c_s = connection_state_.load();
    ConnectionState current_c_s;
    int idx { 0 };
    bool try_again { true };
    bool got_max_backoff { false };
    std::random_device rd;
    std::default_random_engine engine(rd());
    std::uniform_int_distribution<int> dist(-250, 250);

    do {
        current_c_s = connection_state_.load();
        idx++;
        if (max_connect_attempts) {
            try_again = (idx < max_connect_attempts);
        }
        got_max_backoff |= (connection_backoff_ms_ * CONNECTION_BACKOFF_MULTIPLIER
                            >= CONNECTION_BACKOFF_LIMIT);

        switch (current_c_s) {
        case(ConnectionStateValues::initialized):
            assert(previous_c_s == ConnectionStateValues::initialized);
            connect_();
            Util::this_thread::sleep_for(
                Util::chrono::milliseconds(CONNECTION_MIN_INTERVAL));
            break;

        case(ConnectionStateValues::connecting):
            previous_c_s = current_c_s;
            Util::this_thread::sleep_for(
                Util::chrono::milliseconds(CONNECTION_MIN_INTERVAL));
            continue;

        case(ConnectionStateValues::open):
            if (previous_c_s != ConnectionStateValues::open) {
                LOG_INFO("Successfully established a WebSocket connection with "
                         "the PCP broker");
                connection_backoff_ms_ = CONNECTION_BACKOFF_MS;
            }
            return;

        case(ConnectionStateValues::closing):
            previous_c_s = current_c_s;
            Util::this_thread::sleep_for(
                Util::chrono::milliseconds(CONNECTION_MIN_INTERVAL));
            continue;

        case(ConnectionStateValues::closed):
            assert(previous_c_s != ConnectionStateValues::open);
            if (previous_c_s == ConnectionStateValues::closed) {
                connect_();
                Util::this_thread::sleep_for(
                    Util::chrono::milliseconds(CONNECTION_MIN_INTERVAL));
                previous_c_s = ConnectionStateValues::connecting;
            } else {
                LOG_WARNING("Failed to establish a WebSocket connection; "
                            "retrying in %1% seconds",
                            static_cast<int>(connection_backoff_ms_ / 1000));
                // Randomly adjust the interval slightly to help calm a
                // thundering herd
                Util::this_thread::sleep_for(
                    Util::chrono::milliseconds(connection_backoff_ms_ + dist(engine)));
                connect_();
                Util::this_thread::sleep_for(
                    Util::chrono::milliseconds(CONNECTION_MIN_INTERVAL));
                if (try_again && !got_max_backoff) {
                    connection_backoff_ms_ *= CONNECTION_BACKOFF_MULTIPLIER;
                }
            }
            break;
        }
    } while (try_again);

    connection_backoff_ms_ = CONNECTION_BACKOFF_MS;
    throw connection_fatal_error { "failed to establish a WebSocket connection "
                                   "after " + std::to_string(idx) + " attempt"
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
        throw connection_processing_error { "failed to send WebSocket ping: "
                                            + ec.message() };
    }
}

void Connection::close(CloseCode code, const std::string& reason) {
    LOG_DEBUG("About to close connection");
    websocketpp::lib::error_code ec;
    endpoint_->close(connection_handle_, code, reason, ec);
    if (ec) {
        throw connection_processing_error { "failed to close WebSocket "
                                            "connection: " + ec.message() };
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
            LOG_ERROR("Failed to close the WebSocket connection: %1%", e.what());
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
        endpoint_->get_connection(broker_ws_uri_, ec) };
    if (ec) {
        throw connection_processing_error { "failed to establish the WebSocket "
                                            "connection with " + broker_ws_uri_
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
    connection_timings_.close = Util::chrono::high_resolution_clock::now();
    LOG_TRACE("WebSocket connection closed");
    connection_state_ = ConnectionStateValues::closed;
}

void Connection::onFail(WS_Connection_Handle hdl) {
    connection_timings_.close = Util::chrono::high_resolution_clock::now();
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
    connection_timings_.tcp_pre_init = Util::chrono::high_resolution_clock::now();
    LOG_TRACE("WebSocket pre-TCP initialization event");
}

void Connection::onPostTCPInit(WS_Connection_Handle hdl) {
    connection_timings_.tcp_post_init = Util::chrono::high_resolution_clock::now();
    LOG_TRACE("WebSocket post-TCP initialization event");
}

void Connection::onOpen(WS_Connection_Handle hdl) {
    connection_timings_.open = Util::chrono::high_resolution_clock::now();
    connection_timings_.connection_started = true;
    LOG_DEBUG("WebSocket on open event - %1%", connection_timings_.toString());
    LOG_INFO("WebSocket connection established");
    connection_state_ = ConnectionStateValues::open;

    if (onOpen_callback) {
        try {
            onOpen_callback();
            return;
        } catch (std::exception&  e) {
            LOG_ERROR("onOpen callback failure: %1%; closing the "
                      "WebSocket connection", e.what());
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
            LOG_ERROR("onMessage WebSocket callback failure: %1%", e.what());
        } catch (...) {
            LOG_ERROR("onMessage WebSocket callback failure: unexpected error");
        }
    }
}

}  // namespace PCPClient

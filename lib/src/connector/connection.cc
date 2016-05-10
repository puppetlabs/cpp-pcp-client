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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#include <websocketpp/common/connection_hdl.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>
#pragma GCC diagnostic pop

#define LEATHERMAN_LOGGING_NAMESPACE CPP_PCP_CLIENT_LOGGING_PREFIX".connection"

#include <leatherman/logging/logging.hpp>

#include <leatherman/util/timer.hpp>

#include <leatherman/locale/locale.hpp>

#include <cstdio>
#include <iostream>
#include <random>

// TODO(ale): disable assert() once we're confident with the code...
// To disable assert()
// #define NDEBUG
#include <cassert>

namespace PCPClient {

namespace lth_util = leatherman::util;
namespace lth_loc  = leatherman::locale;

//
// Constants
//

static const uint32_t CONNECTION_MIN_INTERVAL_MS { 200 };  // [ms]
static const uint32_t CONNECTION_BACKOFF_LIMIT_MS { 33000 };  // [ms]
static const uint32_t CONNECTION_BACKOFF_MULTIPLIER { 2 };

//
// Connection
//

Connection::Connection(std::string broker_ws_uri,
                       ClientMetadata client_metadata)
        : broker_ws_uri_ { std::move(broker_ws_uri) },
          client_metadata_ { std::move(client_metadata) },
          connection_state_ { ConnectionState::initialized },
          consecutive_pong_timeouts_ { 0 },
          endpoint_ { new WS_Client_Type() }
{
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
        throw connection_config_error { lth_loc::translate("failed to initialize") };
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

inline static void doSleep(int ms = CONNECTION_MIN_INTERVAL_MS) {
    Util::this_thread::sleep_for(Util::chrono::milliseconds(ms));
}

void Connection::connect(int max_connect_attempts) {
    // FSM
    //  - states are ConnectionState:
    //      * initialized - connecting - open - closing - closed
    //  - for the transitions, we assume that the connection_state_:
    //      * can be set to 'initialized' only by the Connection ctor;
    //      * is set to 'connecting' by connect_();
    //      * after a connect_() call, it will become, eventually,
    //        open or closed.

    ConnectionState previous_c_s = connection_state_.load();
    ConnectionState current_c_s;
    int idx { 0 };
    bool try_again { true };
    bool got_max_backoff { false };
    std::random_device rd;
    std::default_random_engine engine { rd() };
    std::uniform_int_distribution<int> dist { -250, 250 };

    do {
        current_c_s = connection_state_.load();
        idx++;
        if (max_connect_attempts)
            try_again = (idx < max_connect_attempts);
        got_max_backoff |= (connection_backoff_ms_ * CONNECTION_BACKOFF_MULTIPLIER
                            >= CONNECTION_BACKOFF_LIMIT_MS);

        switch (current_c_s) {
        case(ConnectionState::initialized):
            assert(previous_c_s == ConnectionState::initialized);
            connectAndWait();
            if (connection_state_.load() == ConnectionState::open)
                return;
            break;

        case(ConnectionState::connecting):
            previous_c_s = ConnectionState::connecting;
            doSleep();
            continue;

        case(ConnectionState::open):
            if (previous_c_s != ConnectionState::open)
                connection_backoff_ms_ = CONNECTION_BACKOFF_MS;
            return;

        case(ConnectionState::closing):
            previous_c_s = ConnectionState::closing;
            doSleep();
            continue;

        case(ConnectionState::closed):
            if (previous_c_s == ConnectionState::closed) {
                previous_c_s = ConnectionState::connecting;
                connectAndWait();
            } else {
                LOG_WARNING("Failed to establish a WebSocket connection; "
                            "retrying in {1} seconds",
                            static_cast<int>(connection_backoff_ms_ / 1000));
                // Randomly adjust the interval slightly to help calm
                // a thundering herd
                doSleep(connection_backoff_ms_ + dist(engine));
                connectAndWait();
                if (try_again && !got_max_backoff) {
                    connection_backoff_ms_ *= CONNECTION_BACKOFF_MULTIPLIER;
                }
            }
            break;
        }
    } while (try_again);

    connection_backoff_ms_ = CONNECTION_BACKOFF_MS;
    // TODO(ale): deal with locale & plural (PCP-257)
    auto msg = (idx == 1) ?
      lth_loc::format("failed to establish a WebSocket connection after {1} attempt", idx) :
      lth_loc::format("failed to establish a WebSocket connection after {1} attempts", idx);
    throw connection_fatal_error { msg };
}

void Connection::send(const std::string& msg) {
    websocketpp::lib::error_code ec;
    endpoint_->send(connection_handle_,
                    msg,
                    websocketpp::frame::opcode::binary,
                    ec);
    if (ec)
        throw connection_processing_error {
            lth_loc::format("failed to send message: {1}", ec.message()) };
}

void Connection::send(void* const serialized_msg_ptr, size_t msg_len) {
    websocketpp::lib::error_code ec;
    endpoint_->send(connection_handle_,
                    serialized_msg_ptr,
                    msg_len,
                    websocketpp::frame::opcode::binary,
                    ec);
    if (ec)
        throw connection_processing_error {
            lth_loc::format("failed to send message: {1}", ec.message()) };
}

void Connection::ping(const std::string& binary_payload) {
    websocketpp::lib::error_code ec;
    endpoint_->ping(connection_handle_, binary_payload, ec);
    if (ec)
        throw connection_processing_error {
            lth_loc::format("failed to send WebSocket ping: {1}",
                            ec.message()) };
}

void Connection::close(CloseCode code, const std::string& reason) {
    LOG_DEBUG("About to close the WebSocket connection");
    websocketpp::lib::error_code ec;
    endpoint_->close(connection_handle_, code, reason, ec);
    if (ec)
        throw connection_processing_error {
            lth_loc::format("failed to close WebSocket connection: {1}",
                            ec.message()) };
}

//
// Private interface
//

void Connection::connectAndWait() {
    static auto connection_timeout_s =
        static_cast<int>(client_metadata_.connection_timeout / 1000);
    connect_();
    lth_util::Timer timer {};
    while (connection_state_.load() != ConnectionState::open
           && timer.elapsed_seconds() < connection_timeout_s) {
        doSleep();
    }
}

void Connection::tryClose() {
    try {
        close();
    } catch (connection_processing_error& e) {
        LOG_WARNING("Cleanup failure: {1}", e.what());
    }
}

void Connection::cleanUp() {
    auto c_s = connection_state_.load();

    if (c_s == ConnectionState::open) {
        tryClose();
    } else if (c_s == ConnectionState::connecting) {
        // Wait connection_timeout ms, in case of a concurrent onOpen
        LOG_WARNING("Will wait {1} ms before terminating the WebSocket connection",
                    client_metadata_.connection_timeout);
        doSleep(client_metadata_.connection_timeout);
        if (connection_state_.load() == ConnectionState::open)
            tryClose();
    }

    endpoint_->stop_perpetual();

    if (endpoint_thread_ != nullptr && endpoint_thread_->joinable()) {
        endpoint_thread_->join();
    }
}

void Connection::connect_() {
    connection_state_ = ConnectionState::connecting;
    connection_timings_ = ConnectionTimings();
    websocketpp::lib::error_code ec;
    WS_Client_Type::connection_ptr connection_ptr {
        endpoint_->get_connection(broker_ws_uri_, ec) };
    if (ec)
        throw connection_processing_error {
            lth_loc::format("failed to establish the WebSocket connection "
                            "with {1}: {2}", broker_ws_uri_, ec.message()) };

    connection_handle_ = connection_ptr->get_handle();
    LOG_INFO("Connecting to '{1}' with a connection timeout of {2} ms",
              broker_ws_uri_, client_metadata_.connection_timeout);
    connection_ptr->set_open_handshake_timeout(client_metadata_.connection_timeout);
    endpoint_->connect(connection_ptr);
}

//
// Event handlers (private)
//

template <typename Verifier>
class verbose_verification {
  public:
    verbose_verification(Verifier verifier)
            : verifier_(verifier)
    {}

    bool operator()(bool preverified,
                    boost::asio::ssl::verify_context& ctx) {
        char subject_name[256], issuer_name[256];
        X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
        X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
        X509_NAME_oneline(X509_get_issuer_name(cert), issuer_name, 256);
        bool verified = verifier_(preverified, ctx);
        LOG_DEBUG("Verifying {1}, issued by {2}. Verified: {3}",
                  subject_name, issuer_name, verified);
        return verified;
    }

  private:
    Verifier verifier_;
};

/// \brief Auxiliary function to make verbose_verification objects.
template <typename Verifier>
static verbose_verification<Verifier>
make_verbose_verification(Verifier verifier)
{
  return verbose_verification<Verifier>(verifier);
}

WS_Context_Ptr Connection::onTlsInit(WS_Connection_Handle hdl) {
    LOG_DEBUG("WebSocket TLS initialization event; about to validate the certificate");
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

        auto uri = websocketpp::uri(broker_ws_uri_);
        ctx->set_verify_mode(boost::asio::ssl::verify_peer);
        ctx->set_verify_callback(
            make_verbose_verification(
                boost::asio::ssl::rfc2818_verification(uri.get_host())));
        LOG_DEBUG("Initialized SSL context to verify broker {1}", uri.get_host());
    } catch (std::exception& e) {
        // This is unexpected, as the CliendMetadata ctor does
        // validate the key / cert pair
        throw connection_config_error {
            lth_loc::format("TLS error: {1}", e.what()) };
    }
    return ctx;
}

void Connection::onClose(WS_Connection_Handle hdl) {
    connection_timings_.close = Util::chrono::high_resolution_clock::now();
    auto con = endpoint_->get_con_from_hdl(hdl);
    LOG_DEBUG("WebSocket on close event: {1} (code: {2}) - {3}",
              con->get_ec().message(), con->get_remote_close_code(),
              connection_timings_.toString());
    connection_state_ = ConnectionState::closed;
}

void Connection::onFail(WS_Connection_Handle hdl) {
    connection_timings_.close = Util::chrono::high_resolution_clock::now();
    connection_timings_.connection_failed = true;
    auto con = endpoint_->get_con_from_hdl(hdl);
    LOG_DEBUG("WebSocket on fail event - {1}", connection_timings_.toString());
    LOG_WARNING("WebSocket on fail event (connection loss): {1} (code: {2})",
                con->get_ec().message(), con->get_remote_close_code());
    connection_state_ = ConnectionState::closed;
}

bool Connection::onPing(WS_Connection_Handle hdl, std::string binary_payload) {
    LOG_TRACE("WebSocket onPing event - payload: {1}", binary_payload);
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
    LOG_WARNING("WebSocket onPongTimeout event ({1} consecutive)",
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
    LOG_DEBUG("WebSocket on open event - {1}", connection_timings_.toString());
    LOG_INFO("Successfully established a WebSocket connection with the PCP "
             "broker at {1}", broker_ws_uri_);
    connection_state_ = ConnectionState::open;

    if (onOpen_callback) {
        try {
            onOpen_callback();
            return;
        } catch (std::exception&  e) {
            LOG_ERROR("onOpen callback failure: {1}; closing the "
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
            LOG_ERROR("onMessage WebSocket callback failure: {1}", e.what());
        } catch (...) {
            LOG_ERROR("onMessage WebSocket callback failure: unexpected error");
        }
    }
}

}  // namespace PCPClient

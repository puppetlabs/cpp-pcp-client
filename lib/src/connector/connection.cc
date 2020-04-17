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
#include <cpp-pcp-client/protocol/v1/message.hpp>
#include <cpp-pcp-client/util/thread.hpp>
#include <cpp-pcp-client/util/chrono.hpp>

// This is hacky because MinGW-w64 5.2 with Boost 1.58 is printing warnings that should be suppressed. Preserve the
// warnings elsewhere to make sure we have coverage of our code, but suppress for the whole file on Windows to avoid
// printing them to stderr (which causes Appveyor builds to fail).
#ifndef _WIN32
#pragma GCC diagnostic push
#endif
#pragma GCC diagnostic ignored "-Wunused-variable"
#include <websocketpp/common/connection_hdl.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>
#ifndef _WIN32
#pragma GCC diagnostic pop
#endif

#define LEATHERMAN_LOGGING_NAMESPACE CPP_PCP_CLIENT_LOGGING_PREFIX".connection"

#include <leatherman/logging/logging.hpp>

#include <leatherman/util/timer.hpp>

#include <leatherman/locale/locale.hpp>

#include <cstdio>
#include <iostream>
#include <random>
#include <algorithm>

// We need to modify underlying openssl object to set CRL.
// These includes exposes methods for reading and validating
// against a CRL.
#include <openssl/x509_vfy.h>

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
        : Connection { std::vector<std::string> { std::move(broker_ws_uri) },
                       std::move(client_metadata) }
{
}

Connection::Connection(std::vector<std::string> broker_ws_uris,
                       ClientMetadata client_metadata)
        : timings {},
          broker_ws_uris_ { std::move(broker_ws_uris) },
          client_metadata_ { std::move(client_metadata) },
          connection_state_ { ConnectionState::initialized },
          connection_target_index_ { 0u },
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
        // Handlers
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

        // Pong timeout
        endpoint_->set_pong_timeout(client_metadata_.pong_timeout_ms);

        // Start the event loop thread
        endpoint_thread_.reset(new Util::thread(&WS_Client_Type::run, endpoint_.get()));
    } catch (...) {
        LOG_DEBUG("Failed to configure the WebSocket endpoint; about to stop "
                  "the event loop");
        cleanUp();
        throw connection_config_error { lth_loc::translate("failed to initialize") };
    }
}

Connection::~Connection()
{
    cleanUp();
}

ConnectionState Connection::getConnectionState() const
{
    return connection_state_.load();
}

//
// Callback modifiers
//

void Connection::setOnOpenCallback(std::function<void()> c_b)
{
    onOpen_callback_ = c_b;
}

void Connection::setOnMessageCallback(std::function<void(const std::string& msg)> c_b)
{
    onMessage_callback_ = c_b;
}

void Connection::setOnCloseCallback(std::function<void()> c_b)
{
    onClose_callback_ = c_b;
}

void Connection::setOnFailCallback(std::function<void()> c_b)
{
    onFail_callback_ = c_b;
}

void Connection::resetCallbacks()
{
    onOpen_callback_    = [](){};  // NOLINT [false positive readability/braces]
    onMessage_callback_ = [](std::string message){};  // NOLINT [false positive readability/braces]
    onClose_callback_   = [](){};  // NOLINT [false positive readability/braces]
    onFail_callback_    = [](){};  // NOLINT [false positive readability/braces]
}

//
// Synchronous calls
//

inline static void doSleep(int ms = CONNECTION_MIN_INTERVAL_MS)
{
    Util::this_thread::sleep_for(Util::chrono::milliseconds(ms));
}

void Connection::connect(int max_connect_attempts)
{
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
    std::uniform_int_distribution<int> dist { -500, 500 };

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
                // Connection attempt failed, next try should be against a failover broker.
                switchWsUri();
                // Randomly adjust the interval slightly to help calm
                // a thundering herd
                doSleep(connection_backoff_ms_ + dist(engine));
                connectAndWait();
                if (try_again && !got_max_backoff) {
                    // exponential backoff with a 1.5-2.5x multiplier up to a max of 32 seconds
                    connection_backoff_ms_ *= static_cast<int>(dist(engine) / 1000 + CONNECTION_BACKOFF_MULTIPLIER);
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

void Connection::send(const std::string& msg)
{
    websocketpp::lib::error_code ec;
    endpoint_->send(connection_handle_,
                    msg,
                    websocketpp::frame::opcode::text,
                    ec);
    if (ec)
        throw connection_processing_error {
            lth_loc::format("failed to send message: {1}", ec.message()) };
}

void Connection::send(void* const serialized_msg_ptr, size_t msg_len)
{
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

void Connection::ping(const std::string& binary_payload)
{
    websocketpp::lib::error_code ec;
    endpoint_->ping(connection_handle_, binary_payload, ec);
    if (ec)
        throw connection_processing_error {
            lth_loc::format("failed to send WebSocket ping: {1}",
                            ec.message()) };
}

void Connection::close(CloseCode code, const std::string& reason)
{
    LOG_DEBUG("About to close the WebSocket connection");
    Util::lock_guard<Util::mutex> the_lock { state_mutex_ };
    timings.setClosing();
    auto c_s = connection_state_.load();
    if (c_s != ConnectionState::closed) {
        websocketpp::lib::error_code ec;
        endpoint_->close(connection_handle_, code, reason, ec);
        if (ec)
            throw connection_processing_error {
                    lth_loc::format("failed to close WebSocket connection: {1}",
                                    ec.message()) };
        connection_state_ = ConnectionState::closing;
    }
}

//
// Private interface
//

void Connection::connectAndWait()
{
    connect_();
    Util::unique_lock<Util::mutex> lck { onOpen_mtx };
    onOpen_cv.wait_for(lck,
                       Util::chrono::milliseconds(client_metadata_.ws_connection_timeout_ms),
                       [this]() -> bool {
                           return connection_state_.load() == ConnectionState::open;
                       });
}

void Connection::tryClose()
{
    try {
        close();
    } catch (connection_processing_error& e) {
        LOG_WARNING("Cleanup failure: {1}", e.what());
    }
}

void Connection::cleanUp()
{
    auto c_s = connection_state_.load();

    switch (c_s) {
        case (ConnectionState::closed):
        case (ConnectionState::initialized):
            break;

        case (ConnectionState::open):
        case (ConnectionState::closing):
        {
            if (c_s == ConnectionState::open) {
                tryClose();
            }
            lth_util::Timer timer{};
            while (connection_state_.load() == ConnectionState::closing
                   && timer.elapsed_seconds() < 2)
                doSleep();
            break;
        }

        case(ConnectionState::connecting):
        {
            // This is unexpected; the underlying WebSocket could be
            // in an invalid state and we may fail to close it
            LOG_WARNING("WebSocket in 'connecting' state; will try to close");
            tryClose();
            if (connection_state_.load() == ConnectionState::closed)
                break;
            // Wait at least 5000 ms, in case of a concurrent onOpen
            auto timeout = std::max<long>(5000,
                                          client_metadata_.ws_connection_timeout_ms);
            LOG_WARNING("Failed to close the WebSocket; will wait at most "
                        "{1} ms before trying again", timeout);
            lth_util::Timer timer{};
            while (connection_state_.load() == ConnectionState::connecting
                   && timer.elapsed_milliseconds() < timeout)
                doSleep();
            tryClose();
        }
    }

    endpoint_->stop_perpetual();

    if (endpoint_thread_ != nullptr && endpoint_thread_->joinable())
        endpoint_thread_->join();
}

void Connection::connect_()
{
    connection_state_ = ConnectionState::connecting;
    timings.reset();
    websocketpp::lib::error_code ec;
    auto ws_uri = getWsUri();
    WS_Client_Type::connection_ptr connection_ptr {
        endpoint_->get_connection(ws_uri, ec) };
    if (ec)
        throw connection_processing_error {
            lth_loc::format("failed to establish the WebSocket connection "
                            "with {1}: {2}", ws_uri, ec.message()) };

    connection_handle_ = connection_ptr->get_handle();
    if (client_metadata_.proxy.length() > 0) {
        connection_ptr->set_proxy(client_metadata_.proxy);
        LOG_INFO("Establishing the WebSocket connection with '{1}'"
                 " through proxy '{2}' with a timeout of {3} ms",
                 ws_uri, client_metadata_.proxy, client_metadata_.ws_connection_timeout_ms);
    } else {
        LOG_INFO("Establishing the WebSocket connection with '{1}' with a timeout of {2} ms",
                  ws_uri, client_metadata_.ws_connection_timeout_ms);
    }
    connection_ptr->set_open_handshake_timeout(client_metadata_.ws_connection_timeout_ms);

    try {
        endpoint_->connect(connection_ptr);
    } catch (const std::exception& e) {
        throw connection_processing_error {
                lth_loc::format("failed to establish the WebSocket connection "
                                "with {1}: {2}", ws_uri, e.what()) };
    }
}

std::string const& Connection::getWsUri()
{
    auto c_t = connection_target_index_.load();
    return broker_ws_uris_[c_t % broker_ws_uris_.size()];
}

void Connection::switchWsUri()
{
    auto old_t = getWsUri();
    ++connection_target_index_;
    auto current_t = getWsUri();
    if (old_t != current_t)
        LOG_WARNING("Failed to connect to {1}; switching to {2}",
                    old_t, current_t);
}

//
// Event handlers (private)
//

template <typename Verifier>
class verbose_verification
{
  public:
    // cppcheck-suppress passedByValue
    verbose_verification(Verifier verifier, std::string uri)
            : verifier_(verifier), uri_(std::move(uri))
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
        if (!verified) {
            // Issue a warning. Avoid throwing exceptions because this will be called
            // by OpenSSL, and that code likely doesn't handle cleanup on exceptions.
            LOG_WARNING("TLS handshake failed, no subject name matching {1} found, or ca mismatch", uri_);
        }
        return verified;
    }

  private:
    Verifier verifier_;
    std::string uri_;
};

/// \brief Auxiliary function to make verbose_verification objects.
template <typename Verifier>
static verbose_verification<Verifier>
make_verbose_verification(Verifier verifier, std::string uri)
{
    return verbose_verification<Verifier>(verifier, std::move(uri));
}

WS_Context_Ptr Connection::onTlsInit(WS_Connection_Handle hdl)
{
    LOG_DEBUG("WebSocket TLS initialization event; about to validate the certificate");
    // NB: for TLS certificates, refer to:
    // www.boost.org/doc/libs/1_56_0/doc/html/boost_asio/reference/ssl__context.html
    WS_Context_Ptr ctx {
        new boost::asio::ssl::context(boost::asio::ssl::context::tlsv12_client) };
    try {
        // no_sslv2 and no_sslv3 here are not strictly necessary, as the tlsv1 method above
        // ensures we will not try to initiate any connection below TLSv1. However, this avoids
        // any ambiguity in what we support.
        ctx->set_options(boost::asio::ssl::context::default_workarounds |
                         boost::asio::ssl::context::no_sslv2 |
                         boost::asio::ssl::context::no_sslv3 |
                         boost::asio::ssl::context::single_dh_use);
        ctx->use_certificate_file(client_metadata_.crt,
                                  boost::asio::ssl::context::file_format::pem);
        ctx->use_private_key_file(client_metadata_.key,
                                  boost::asio::ssl::context::file_format::pem);
        ctx->load_verify_file(client_metadata_.ca);

        if (client_metadata_.crl.length() > 0) {
            LOG_DEBUG("Using CRL file: {1}", client_metadata_.crl);
            auto x509_store = SSL_CTX_get_cert_store(ctx->native_handle());
            X509_LOOKUP* lu = X509_STORE_add_lookup(x509_store, X509_LOOKUP_file());
            // Returns the number of objects loaded from CRL file or 0 on error
            if (X509_load_crl_file(lu, client_metadata_.crl.c_str(), X509_FILETYPE_PEM) == 0) {
                throw connection_config_error {
                    lth_loc::format("Cannot load crl file: {1}", client_metadata_.crl) };
            }
            X509_STORE_set_flags(x509_store, (X509_V_FLAG_CRL_CHECK_ALL | X509_V_FLAG_CRL_CHECK));
        }
        auto uri_txt = getWsUri();
        auto uri = websocketpp::uri(uri_txt);
        ctx->set_verify_mode(boost::asio::ssl::verify_peer);
        ctx->set_verify_callback(
            make_verbose_verification(
                boost::asio::ssl::rfc2818_verification(uri.get_host()), uri_txt));
        LOG_DEBUG("Initialized SSL context to verify broker {1}", uri.get_host());
    } catch (std::exception& e) {
        // This is unexpected, as the CliendMetadata ctor does
        // validate the key / cert pair
        throw connection_config_error {
            lth_loc::format("TLS error: {1}", e.what()) };
    }
    return ctx;
}

void Connection::onClose(WS_Connection_Handle hdl)
{
    Util::lock_guard<Util::mutex> the_lock { state_mutex_ };
    timings.setClosed();
    auto con = endpoint_->get_con_from_hdl(hdl);
    auto close_code = con->get_remote_close_code();

    if (close_code == 1000) {
        // Normal closure; don't log error code
        LOG_DEBUG("WebSocket on close event (normal) - {1}", timings.toString());
    } else {
        LOG_DEBUG("WebSocket on close event: {1} (code: {2}) - {3}",
                  con->get_ec().message(), close_code, timings.toString());
    }

    if (timings.isClosingStarted())
        LOG_DEBUG("WebSocket on close event - Closing Handshake {1} us",
                  timings.getClosingHandshakeInterval().count());

    connection_state_ = ConnectionState::closed;

    if (onClose_callback_) {
        try {
            onClose_callback_();
        } catch (std::exception&  e) {
            LOG_ERROR("onClose WebSocket callback failure: {1}", e.what());
        } catch (...) {
            LOG_ERROR("onClose WebSocket callback failure: unexpected error");
        }
    }
}

void Connection::onFail(WS_Connection_Handle hdl)
{
    Util::lock_guard<Util::mutex> the_lock { state_mutex_ };
    timings.setClosed(true);
    auto con = endpoint_->get_con_from_hdl(hdl);
    LOG_DEBUG("WebSocket on fail event - {1}", timings.toString());
    LOG_WARNING("WebSocket on fail event (connection loss): {1} (code: {2})",
                con->get_ec().message(), con->get_remote_close_code());
    connection_state_ = ConnectionState::closed;

    if (onFail_callback_) {
        try {
            onFail_callback_();
        } catch (std::exception&  e) {
            LOG_ERROR("onFail WebSocket callback failure: {1}", e.what());
        } catch (...) {
            LOG_ERROR("onFail WebSocket callback failure: unexpected error");
        }
    }
}

bool Connection::onPing(WS_Connection_Handle hdl, std::string binary_payload)
{
    LOG_TRACE("WebSocket onPing event - payload: {1}", binary_payload);
    // Returning true so the transport layer will send back a pong
    return true;
}

void Connection::onPong(WS_Connection_Handle hdl, std::string binary_payload)
{
    LOG_DEBUG("WebSocket onPong event");
    if (consecutive_pong_timeouts_) {
        consecutive_pong_timeouts_ = 0;
    }
}

void Connection::onPongTimeout(WS_Connection_Handle hdl,
                               std::string binary_payload)
{
    ++consecutive_pong_timeouts_;
    if (consecutive_pong_timeouts_ >= client_metadata_.pong_timeouts_before_retry) {
        LOG_WARNING("WebSocket onPongTimeout event ({1} consecutive); "
                    "closing the WebSocket connection", consecutive_pong_timeouts_);
        close(CloseCodeValues::normal, "consecutive onPongTimeouts");
    } else if (consecutive_pong_timeouts_ == 1) {
        LOG_WARNING("WebSocket onPongTimeout event");
    } else {
        LOG_WARNING("WebSocket onPongTimeout event ({1} consecutive)",
                    consecutive_pong_timeouts_);
    }
}

void Connection::onPreTCPInit(WS_Connection_Handle hdl)
{
    timings.tcp_pre_init = Util::chrono::high_resolution_clock::now();
    LOG_TRACE("WebSocket pre-TCP initialization event");
}

void Connection::onPostTCPInit(WS_Connection_Handle hdl)
{
    timings.tcp_post_init = Util::chrono::high_resolution_clock::now();
    LOG_TRACE("WebSocket post-TCP initialization event");
}

void Connection::onOpen(WS_Connection_Handle hdl) {
    timings.setOpen();
    LOG_DEBUG("WebSocket on open event - {1}", timings.toString());
    LOG_INFO("Successfully established a WebSocket connection with the PCP "
             "broker at {1}", getWsUri());

    {
        Util::lock_guard<Util::mutex> { onOpen_mtx };
        connection_state_ = ConnectionState::open;
    }
    onOpen_cv.notify_one();

    if (onOpen_callback_) {
        try {
            onOpen_callback_();
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
                           WS_Client_Type::message_ptr msg)
{
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

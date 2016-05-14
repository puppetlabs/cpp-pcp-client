// See http://www.zaphoyd.com/websocketpp/manual/reference/cpp11-support
// for prepocessor directives and choosing between boost and cpp11
// C++11 STL tokens
#define _WEBSOCKETPP_CPP11_FUNCTIONAL_
#define _WEBSOCKETPP_CPP11_MEMORY_
#define _WEBSOCKETPP_CPP11_RANDOM_DEVICE_
#define _WEBSOCKETPP_CPP11_SYSTEM_ERROR_
#define _WEBSOCKETPP_NO_CPP11_THREAD_

#include "mock_server.hpp"
#include "certs.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#include <websocketpp/config/asio.hpp>
#include <websocketpp/server.hpp>
#pragma GCC diagnostic pop

#include <cpp-pcp-client/util/thread.hpp>

namespace PCPClient {

MockServer::MockServer(uint16_t port,
                       std::string certPath,
                       std::string keyPath)
    : certPath_(std::move(certPath)), keyPath_(std::move(keyPath)), server_(new websocketpp::server<websocketpp::config::asio_tls>())
{
    namespace asio = websocketpp::lib::asio;
    server_->init_asio();
    server_->set_tls_init_handler([&](websocketpp::connection_hdl hdl) {
        auto ctx = websocketpp::lib::make_shared<asio::ssl::context>(asio::ssl::context::tlsv1);
        ctx->set_options(asio::ssl::context::default_workarounds |
                         asio::ssl::context::no_sslv2 |
                         asio::ssl::context::no_sslv3 |
                         asio::ssl::context::single_dh_use);
        ctx->use_certificate_file(certPath_, asio::ssl::context::file_format::pem);
        ctx->use_private_key_file(keyPath_, asio::ssl::context::file_format::pem);
        return ctx;
    });
    server_->listen(port);
}

MockServer::~MockServer()
{
    server_->stop();
    bt_->join();
}

void MockServer::go()
{
    server_->start_accept();
    bt_.reset(new boost::thread(&websocketpp::server<websocketpp::config::asio_tls>::run, server_.get()));
}

uint16_t MockServer::port()
{
    websocketpp::lib::asio::error_code ec;
    auto ep = server_->get_local_endpoint(ec);
    return ec ? 0 : ep.port();
}

void MockServer::set_open_handler(std::function<void(websocketpp::connection_hdl)> func)
{
    server_->set_open_handler(func);
}

}

// See http://www.zaphoyd.com/websocketpp/manual/reference/cpp11-support
// for prepocessor directives and choosing between boost and cpp11
// C++11 STL tokens
#define _WEBSOCKETPP_CPP11_FUNCTIONAL_
#define _WEBSOCKETPP_CPP11_MEMORY_
#define _WEBSOCKETPP_CPP11_RANDOM_DEVICE_
#define _WEBSOCKETPP_CPP11_SYSTEM_ERROR_
#define _WEBSOCKETPP_NO_CPP11_THREAD_

#include "tests/test.hpp"
#include "tests/unit/connector/certs.hpp"
#include "tests/unit/connector/mock_server.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#include <websocketpp/config/asio.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/common/connection_hdl.hpp>
#pragma GCC diagnostic pop

#include <cpp-pcp-client/util/thread.hpp>
#include <cpp-pcp-client/protocol/schemas.hpp>
#include <cpp-pcp-client/protocol/message.hpp>
#include <cpp-pcp-client/protocol/errors.hpp>
#include <cpp-pcp-client/protocol/chunks.hpp>

#include <leatherman/json_container/json_container.hpp>

#define LEATHERMAN_LOGGING_NAMESPACE CPP_PCP_CLIENT_LOGGING_PREFIX".mock_server"

#include <leatherman/logging/logging.hpp>

#include <iostream>
#include <string>

namespace PCPClient {

namespace lth_jc  = leatherman::json_container;

MockServer::MockServer(uint16_t port,
                       std::string certPath,
                       std::string keyPath)
    : certPath_(std::move(certPath)),
      keyPath_(std::move(keyPath)),
      server_(new websocketpp::server<websocketpp::config::asio_tls>())
{
    // NB: ENABLE_MOCKSERVER_WEBSOCKETPP_LOGGING could be defined in test.hpp
#ifndef ENABLE_MOCKSERVER_WEBSOCKETPP_LOGGING
    server_->clear_access_channels(websocketpp::log::alevel::all);
    server_->clear_error_channels(websocketpp::log::elevel::all);
#endif

    namespace asio = websocketpp::lib::asio;
    server_->init_asio();

    server_->set_tls_init_handler(
        [&](websocketpp::connection_hdl hdl) {
            auto ctx = websocketpp::lib::make_shared<asio::ssl::context>(asio::ssl::context::tlsv1);
            ctx->set_options(asio::ssl::context::default_workarounds |
                             asio::ssl::context::no_sslv2 |
                             asio::ssl::context::no_sslv3 |
                             asio::ssl::context::single_dh_use);
            ctx->use_certificate_file(certPath_, asio::ssl::context::file_format::pem);
            ctx->use_private_key_file(keyPath_, asio::ssl::context::file_format::pem);
            return ctx;
        });

    // Set the onMessage handler specifically for Association Requests
    server_->set_message_handler(
            std::bind(&MockServer::association_request_handler,
                      this,
                      std::placeholders::_1,
                      std::placeholders::_2));

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

void MockServer::set_ping_handler(std::function<bool(websocketpp::connection_hdl, std::string)> func)
{
    server_->set_ping_handler(func);
}

//
// Private
//

static const std::string MESSAGE_TYPE {"message_type"};
static const std::string ID {"id"};
static const std::string SUCCESS {"success"};

void MockServer::association_request_handler
        (websocketpp::connection_hdl hdl,
         std::shared_ptr<
            class websocketpp::message_buffer::message<
                websocketpp::message_buffer::alloc::con_msg_manager>> req)
{
    try {
        // Deserialize
        Message request {req->get_payload()};

        // Inspect envelope
        lth_jc::JsonContainer env {request.getEnvelopeChunk().content};
        auto message_type = env.get<std::string>(MESSAGE_TYPE);

        if (message_type != Protocol::ASSOCIATE_REQ_TYPE) {
            LOG_ERROR("Unknown message type: {1}", message_type);
            return;
        }

        // Create the response message
        env.set<std::string>(MESSAGE_TYPE, Protocol::ASSOCIATE_RESP_TYPE);
        lth_jc::JsonContainer data {};
        data.set<std::string>(ID, env.get<std::string>(ID));
        env.set<std::string>(ID, "42424242");
        data.set<bool>(SUCCESS, true);
        auto data_txt = data.toString();
        MessageChunk env_chunk  {ChunkDescriptor::ENVELOPE, env.toString()};
        MessageChunk data_chunk {ChunkDescriptor::DATA, data_txt};
        Message resp {env_chunk, data_chunk};
        auto serialized_resp = resp.getSerialized();

        server_->send(hdl,
                      &serialized_resp[0],
                      serialized_resp.size(),
                      websocketpp::frame::opcode::binary);
    } catch (const message_error& e) {
        LOG_ERROR("Failed to deserialize message: {1}", e.what());
        return;
    } catch (const lth_jc::data_error&) {
        LOG_ERROR("Invalid message (bad envelope)");
        return;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to send the Association response: {1}", e.what());
    }
}

}

#pragma once

#include <websocketpp/config/asio_client.hpp>

namespace PCPClient {
  struct ws_config : public websocketpp::config::asio_tls_client {
    static const websocketpp::log::level elog_level =
        websocketpp::log::elevel::all;
    static const websocketpp::log::level alog_level =
        websocketpp::log::alevel::all;
  };
}

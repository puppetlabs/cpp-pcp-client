#include <memory>
#include <functional>
#include <string>

#include "certs.hpp"

namespace boost {
    class thread;
}

namespace websocketpp {
    template <typename T>
    class server;

    namespace config {
        struct asio_tls;
    }

    namespace message_buffer {
        namespace alloc {
            template <typename message>
            class con_msg_manager;
        }

        template <template<class> class con_msg_manager>
        class message;
    }

    using connection_hdl = std::weak_ptr<void>;
}

namespace PCPClient {

class MockServer
{
public:
    MockServer(uint16_t port = 0,
               std::string certPath = getCertPath(),
               std::string keyPath = getKeyPath());
    ~MockServer();
    void go();
    uint16_t port();

    void set_tcp_pre_init_handler(std::function<void(websocketpp::connection_hdl)> func);

    void set_open_handler(std::function<void(websocketpp::connection_hdl)> func);

    void set_ping_handler(std::function<bool(websocketpp::connection_hdl,
                                             std::string)> func);

private:
    std::string certPath_, keyPath_;
    std::unique_ptr<boost::thread> bt_;
    std::unique_ptr<websocketpp::server<websocketpp::config::asio_tls>> server_;

    void association_request_handler(
        websocketpp::connection_hdl hdl,
        std::shared_ptr<
            class websocketpp::message_buffer::message<
                websocketpp::message_buffer::alloc::con_msg_manager>> req);
};

}

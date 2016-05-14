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

    void set_open_handler(std::function<void(websocketpp::connection_hdl)> func);

private:
    std::string certPath_, keyPath_;
    std::unique_ptr<boost::thread> bt_;
    std::unique_ptr<websocketpp::server<websocketpp::config::asio_tls>> server_;
};

}

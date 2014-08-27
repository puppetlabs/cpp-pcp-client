#ifndef CTHUN_CLIENT_SRC_TEST_CLIENT_H_
#define CTHUN_CLIENT_SRC_TEST_CLIENT_H_

#include "client.h"

namespace Cthun {
namespace Client {

//
// Test Client
//

class TestClient : public BaseClient {
  public:
    TestClient() = delete;
    explicit TestClient(std::vector<Message> & messages);
    ~TestClient();

    /// Event loop open callback.
    void onOpen_(websocketpp::connection_hdl hdl);

    /// Event loop close callback.
    void onClose_(websocketpp::connection_hdl hdl);

    /// Event loop failure callback.
    void onFail_(websocketpp::connection_hdl hdl);

    /// Event loop TLS init callback.
    Context_Ptr onTlsInit_(websocketpp::connection_hdl hdl);

    /// Event loop failure callback.
    void onMessage_(websocketpp::connection_hdl hdl,
                    Client_Configuration::message_ptr msg);

  private:
    std::vector<Message> & messages_;
};

}  // namespace Client
}  // namespace Cthun

#endif  // CTHUN_CLIENT_SRC_TEST_CLIENT_H_

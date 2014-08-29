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
    void onOpen_(Connection_Handle hdl);

    /// Event loop close callback.
    void onClose_(Connection_Handle hdl);

    /// Event loop failure callback.
    void onFail_(Connection_Handle hdl);

  private:
    std::vector<Message> & messages_;
};

}  // namespace Client
}  // namespace Cthun

#endif  // CTHUN_CLIENT_SRC_TEST_CLIENT_H_

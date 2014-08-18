#ifndef CTHUN_TEST_CLIENT_SRC_CONNECTION_METADATA_H_
#define CTHUN_TEST_CLIENT_SRC_CONNECTION_METADATA_H_

#include <string>

namespace Cthun_Test_Client {

class ConnectionMetadata {
  public:
    ConnectionMetadata() = delete;
    explicit ConnectionMetadata(std::string url);

  private:
    std::string url_;
};

}  // namespace Cthun_Test_Client

#endif  // CTHUN_TEST_CLIENT_SRC_CONNECTION_METADATA_H_

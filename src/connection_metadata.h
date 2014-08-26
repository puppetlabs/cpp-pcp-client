#ifndef CTHUN_CLIENT_SRC_CONNECTION_METADATA_H_
#define CTHUN_CLIENT_SRC_CONNECTION_METADATA_H_

#include <string>

namespace Cthun {

class ConnectionMetadata {
  public:
    ConnectionMetadata() = delete;
    explicit ConnectionMetadata(std::string url);

  private:
    std::string url_;
};

}  // namespace Cthun

#endif  // CTHUN_CLIENT_SRC_CONNECTION_METADATA_H_

#ifndef CTHUN_CLIENT_SRC_CONNECTION_METADATA_H_
#define CTHUN_CLIENT_SRC_CONNECTION_METADATA_H_

namespace Cthun {
namespace Client {

// TODO(ale): connection state attribute, to be set by event callbacks;
// will require lock - must check overhead impact

class ConnectionMetadata {
  public:
    ConnectionMetadata();
};

}  // namespace Client
}  // namespace Cthun

#endif  // CTHUN_CLIENT_SRC_CONNECTION_METADATA_H_

#ifndef CTHUN_CLIENT_SRC_CONNECTOR_CLIENT_METADATA_H_
#define CTHUN_CLIENT_SRC_CONNECTOR_CLIENT_METADATA_H_

#include <string>

namespace CthunClient {

class ClientMetadata {
  public:
    std::string type;
    std::string ca;
    std::string crt;
    std::string key;
    std::string id;

    /// Throws a connection_config_error in case: the client
    /// certificate file does not exist or is invalid; it fails to
    /// retrieve the client identity from the file
    ClientMetadata(const std::string& _type,
                   const std::string& _ca,
                   const std::string& _crt,
                   const std::string& _key);
};

}  // namespace CthunClient

#endif  // CTHUN_CLIENT_SRC_CONNECTOR_CLIENT_METADATA_H_

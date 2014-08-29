#ifndef CTHUN_CLIENT_SRC_ENDPOINT_H_
#define CTHUN_CLIENT_SRC_ENDPOINT_H_

#include "client.h"
#include "connection.h"

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <thread>

namespace Cthun {
namespace Client {

//
// Endpoint
//

class Endpoint {
  public:
    using Ptr = std::shared_ptr<Endpoint>;

    // TODO(ale): set websocketpp logs in the constructor
    Endpoint();
    ~Endpoint();

    void open(Connection::Ptr connection_ptr);

    // TODO(ale): send binary message; Message class;

    /// Send message on the specified connection.
    /// Throw a message_error in case of failure.
    void send(Connection::Ptr connection_ptr, std::string msg);

    /// Close the specified connection; reason and code are optional
    /// (respectively default to empty string and normal as rfc6455).
    /// Throw a connection_error in case of failure.
    void close(Connection::Ptr connection_ptr,
               Close_Code code = Close_Code_Values::normal,
               Message reason = DEFAULT_CLOSE_REASON);

    /// Close all connections.
    /// Throw a close_error in case of failure.
    void closeConnections();

    // Event loop TLS init callback.
    Context_Ptr onTlsInit(Connection_Handle hdl);

  private:
    Client_Type client_;
    // Kepp track of connections to close them all when needed
    std::map<Connection_ID, Connection::Ptr> connections_;
    std::shared_ptr<std::thread> endpoint_thread_;
};

}  // namespace Client
}  // namespace Cthun

#endif  // CTHUN_CLIENT_SRC_ENDPOINT_H_

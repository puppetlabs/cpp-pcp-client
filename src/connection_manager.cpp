#include "connection_manager.h"
#include "errors.h"

namespace Cthun {
namespace Client {

ConnectionManager& ConnectionManager::Instance() {
    static ConnectionManager instance;
    return instance;
}

void ConnectionManager::configureSecureEndpoint(const std::string& ca_crt_path,
                                                const std::string& client_crt_path,
                                                const std::string& client_key_path) {
    if (endpoint_running_) {
        // TODO(ale): should we throw a client_error to let the caller
        // reset the endpoint?
        std::cout << "### WARNING: an endpoint has already started\n";
    }
    is_secure_ = true;
    ca_crt_path_ = ca_crt_path;
    client_crt_path_ = client_crt_path;
    client_key_path_ = client_key_path;
}

Connection::Ptr ConnectionManager::createConnection(std::string url) {
    return Connection::Ptr(new Connection(url));
}

void ConnectionManager::open(Connection::Ptr connection_ptr) {
    startEndpointIfNeeded();
    endpoint_ptr_->open(connection_ptr);
}

void ConnectionManager::send(Connection::Ptr connection_ptr, Message message) {
    startEndpointIfNeeded();
    endpoint_ptr_->send(connection_ptr, message);
}

void ConnectionManager::close(Connection::Ptr connection_ptr,
                              Close_Code code, Message reason) {
    startEndpointIfNeeded();
    endpoint_ptr_->close(connection_ptr, code, reason);
}

void ConnectionManager::closeAllConnections() {
    if (endpoint_running_) {
        endpoint_ptr_->closeConnections();
    }
}

//
// Private interface
//

ConnectionManager::ConnectionManager()
    : endpoint_ptr_ { nullptr },
      endpoint_running_ { false } {
}

void ConnectionManager::startEndpointIfNeeded() {
    // TODO(ale): logs
    if (!endpoint_running_) {
        endpoint_running_ = true;
        if (is_secure_) {
            endpoint_ptr_.reset(
                new Endpoint(ca_crt_path_, client_crt_path_, client_key_path_));
        } else {
            endpoint_ptr_.reset(new Endpoint());
        }
    }
}


}  // namespace Client
}  // namespace Cthun

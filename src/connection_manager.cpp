#include "connection_manager.h"

namespace Cthun {
namespace Client {

ConnectionManager& ConnectionManager::Instance() {
    static ConnectionManager instance;
    return instance;
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
    if (!endpoint_running_) {
        endpoint_running_ = true;
        endpoint_ptr_.reset(new Endpoint());
    }
}


}  // namespace Client
}  // namespace Cthun

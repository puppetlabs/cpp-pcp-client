#ifndef CTHUN_CLIENT_SRC_CONNECTOR_ERRORS_H_
#define CTHUN_CLIENT_SRC_CONNECTOR_ERRORS_H_

#include <stdexcept>
#include <string>

namespace CthunClient {

/// Base error class.
class connector_error : public std::runtime_error {
  public:
    explicit connector_error(std::string const& msg) : std::runtime_error(msg) {}
};

// TODO(ale): ensure error names are meaningful with the new library

/// Endpoint error class.
class endpoint_error : public connector_error {
  public:
    explicit endpoint_error(std::string const& msg) : connector_error(msg) {}
};

/// Connection error class.
class connection_error : public connector_error {
  public:
    explicit connection_error(std::string const& msg) : connector_error(msg) {}
};

/// Message sending error class.
class message_error : public connector_error {
  public:
    explicit message_error(std::string const& msg) : connector_error(msg) {}
};

/// File not found error class.
class file_not_found_error : public connector_error {
  public:
    explicit file_not_found_error(std::string const& msg) : connector_error(msg) {}
};

}  // namespace CthunClient

#endif  // CTHUN_CLIENT_SRC_CONNECTOR_ERRORS_H_

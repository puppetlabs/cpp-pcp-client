#ifndef CTHUN_CLIENT_SRC_CONNECTOR_ERRORS_H_
#define CTHUN_CLIENT_SRC_CONNECTOR_ERRORS_H_

#include <stdexcept>
#include <string>

namespace CthunClient {

/// Base error class.
class connection_error : public std::runtime_error {
  public:
    explicit connection_error(std::string const& msg)
            : std::runtime_error(msg) {}
};

/// Connection fatal error.
class connection_fatal_error : public connection_error {
  public:
    explicit connection_fatal_error(std::string const& msg)
            : connection_error(msg) {}
};

/// Connection configuration error.
class connection_config_error : public connection_error {
  public:
    explicit connection_config_error(std::string const& msg)
            : connection_error(msg) {}
};

/// Connection processing error.
class connection_processing_error : public connection_error {
  public:
    explicit connection_processing_error(std::string const& msg)
            : connection_error(msg) {}
};

/// Connection not initialized error.
class connection_not_init_error : public connection_error {
  public:
    explicit connection_not_init_error(std::string const& msg)
            : connection_error(msg) {}
};

}  // namespace CthunClient

#endif  // CTHUN_CLIENT_SRC_CONNECTOR_ERRORS_H_

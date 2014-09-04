#ifndef CTHUN_CLIENT_SRC_ERRORS_H_
#define CTHUN_CLIENT_SRC_ERRORS_H_

#include <stdexcept>
#include <string>

namespace Cthun {
namespace Client {

/// Base error class.
class client_error : public std::runtime_error {
  public:
    explicit client_error(std::string const& msg) : std::runtime_error(msg) {}
};

/// Connection error class.
class connection_error : public client_error {
  public:
    explicit connection_error(std::string const& msg) : client_error(msg) {}
};

/// Message sending error class.
class message_error : public client_error {
  public:
    explicit message_error(std::string const& msg) : client_error(msg) {}
};

}  // namespace Client
}  // namespace Cthun

#endif  // CTHUN_CLIENT_SRC_ERRORS_H_

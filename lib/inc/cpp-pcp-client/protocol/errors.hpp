#ifndef CTHUN_CLIENT_SRC_PROTOCOL_ERRORS_H_
#define CTHUN_CLIENT_SRC_PROTOCOL_ERRORS_H_

#include <stdexcept>
#include <string>

namespace CthunClient {

/// Base error class.
class message_error : public std::runtime_error {
  public:
    explicit message_error(std::string const& msg)
            : std::runtime_error(msg) {}
};

/// Serialization error
class message_serialization_error : public message_error {
  public:
    explicit message_serialization_error(std::string const& msg)
            : message_error(msg) {}
};

/// Unsupported version error
class unsupported_version_error : public message_error {
  public:
    explicit unsupported_version_error(std::string const& msg)
            : message_error(msg) {}
};

/// Invalid chunk error
class invalid_chunk_error : public message_error {
  public:
    explicit invalid_chunk_error(std::string const& msg)
            : message_error(msg) {}
};

}  // namespace CthunClient

#endif  // CTHUN_CLIENT_SRC_PROTOCOL_ERRORS_H_

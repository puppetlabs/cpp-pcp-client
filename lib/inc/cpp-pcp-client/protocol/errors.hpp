#ifndef CPP_PCP_CLIENT_SRC_PROTOCOL_ERRORS_H_
#define CPP_PCP_CLIENT_SRC_PROTOCOL_ERRORS_H_

#include <cpp-pcp-client/export.h>
#include <stdexcept>
#include <string>

namespace PCPClient {

/// Base error class.
class LIBCPP_PCP_CLIENT_EXPORT message_error : public std::runtime_error {
  public:
    explicit message_error(std::string const& msg)
            : std::runtime_error(msg) {}
};

/// Serialization error
class LIBCPP_PCP_CLIENT_EXPORT message_serialization_error : public message_error {
  public:
    explicit message_serialization_error(std::string const& msg)
            : message_error(msg) {}
};

/// Unsupported version error
class LIBCPP_PCP_CLIENT_EXPORT unsupported_version_error : public message_error {
  public:
    explicit unsupported_version_error(std::string const& msg)
            : message_error(msg) {}
};

/// Invalid chunk error
class LIBCPP_PCP_CLIENT_EXPORT invalid_chunk_error : public message_error {
  public:
    explicit invalid_chunk_error(std::string const& msg)
            : message_error(msg) {}
};

}  // namespace PCPClient

#endif  // CPP_PCP_CLIENT_SRC_PROTOCOL_ERRORS_H_

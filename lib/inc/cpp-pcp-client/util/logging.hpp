#pragma once

#include <cpp-pcp-client/export.h>

#include <memory>
#include <ostream>

// Forward declaration for leatherman::logging::log_level
namespace leatherman {
    namespace logging {
        enum class log_level;
    }  // namespace leatherman
}  // namespace logging

/* This header provides a utility to setup logging for the cpp-pcp-client library.
   When Boost.Log is statically linked, logging configuration has to be done from
   within the cpp-pcp-client library for logging to work.
*/

#define LOG_ACCESS(message) PCPClient::Util::logAccess(message);

namespace PCPClient {
namespace Util {

void logAccess(std::string const& message);

LIBCPP_PCP_CLIENT_EXPORT
void setupLogging(std::ostream &stream,
                  bool force_colorization,
                  std::string const& loglevel_label,
                  std::shared_ptr<std::ostream> access_stream = nullptr);

LIBCPP_PCP_CLIENT_EXPORT
void setupLogging(std::ostream &log_stream,
                  bool force_colorization,
                  leatherman::logging::log_level const& lvl,
                  std::shared_ptr<std::ostream> access_stream = nullptr);

}  // namespace Util
}  // namespace PCPClient

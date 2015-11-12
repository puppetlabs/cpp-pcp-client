#pragma once

#include <cpp-pcp-client/export.h>
#include <ostream>

/* This header provides a utility to setup logging for the cpp-pcp-client library.
   When Boost.Log is statically linked, logging configuration has to be done from
   within the cpp-pcp-client library for logging to work.
*/

namespace PCPClient {
namespace Util {

LIBCPP_PCP_CLIENT_EXPORT void setupLogging(std::ostream &stream,
                  bool force_colorization,
                  std::string const& loglevel_label);

}  // namespace Util
}  // namespace PCPClient

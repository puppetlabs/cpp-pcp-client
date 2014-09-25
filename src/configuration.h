#ifndef CTHUN_CLIENT_SRC_CONFIGURATION_H_
#define CTHUN_CLIENT_SRC_CONFIGURATION_H_

#include "log/log.h"

#include <iostream>

namespace Cthun {
namespace Client {
namespace Configuration {

static const Log::log_level DEFAULT_LOG_LEVEL { Log::log_level::info };
static std::ostream& DEFAULT_LOG_STREAM = std::cout ;

// TODO(ale): allow configuring log to file

/// Configure the default logging.
void initializeLogging(
    Log::log_level log_level = DEFAULT_LOG_LEVEL,
    std::ostream& log_stream = DEFAULT_LOG_STREAM);

}  // namespace Configuration
}  // namespace Client
}  // namespace Cthun

#endif  // CTHUN_CLIENT_SRC_CONFIGURATION_H_

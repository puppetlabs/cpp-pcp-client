#include "configuration.h"

LOG_DECLARE_NAMESPACE("client.configuration");

namespace Cthun {
namespace Client {

void Configuration::initializeLogging(Log::log_level log_level,
                                      std::ostream& log_stream) {
    Log::configure_logging(log_level, log_stream);
}

}  // namespace Client
}  // namespace Cthun

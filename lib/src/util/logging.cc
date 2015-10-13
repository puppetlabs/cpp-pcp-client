#include <cpp-pcp-client/util/logging.hpp>
#define LEATHERMAN_LOGGING_NAMESPACE "puppetlabs.pcp_client.configuration"
#include <leatherman/logging/logging.hpp>
#include <map>

namespace PCPClient {
namespace Util {

namespace lth_log = leatherman::logging;

void setupLogging(std::ostream &stream, bool color, std::string const& level)
{
    const std::map<std::string, lth_log::log_level> option_to_log_level {
        { "none", lth_log::log_level::none },
        { "trace", lth_log::log_level::trace },
        { "debug", lth_log::log_level::debug },
        { "info", lth_log::log_level::info },
        { "warning", lth_log::log_level::warning },
        { "error", lth_log::log_level::error },
        { "fatal", lth_log::log_level::fatal }
    };
    auto lvl = option_to_log_level.at(level);

    lth_log::setup_logging(stream);
    lth_log::set_colorization(color);
    lth_log::set_level(lvl);
}

}  // namespace Util
}  // namespace PCPClient

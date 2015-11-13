#include <cpp-pcp-client/util/logging.hpp>

#define LEATHERMAN_LOGGING_NAMESPACE "puppetlabs.pcp_client.configuration"
#include <leatherman/logging/logging.hpp>

#include <map>

namespace PCPClient {
namespace Util {

namespace lth_log = leatherman::logging;

static void setupLoggingImp(std::ostream &stream,
                            bool force_colorization,
                            lth_log::log_level const& lvl) {
    lth_log::setup_logging(stream);
    lth_log::set_level(lvl);
    if (force_colorization) {
        lth_log::set_colorization(true);
    }
}

void setupLogging(std::ostream &stream,
                  bool force_colorization,
                  std::string const& loglevel_label) {
    const std::map<std::string, lth_log::log_level> label_to_log_level {
        { "none", lth_log::log_level::none },
        { "trace", lth_log::log_level::trace },
        { "debug", lth_log::log_level::debug },
        { "info", lth_log::log_level::info },
        { "warning", lth_log::log_level::warning },
        { "error", lth_log::log_level::error },
        { "fatal", lth_log::log_level::fatal }
    };

    auto lvl = label_to_log_level.at(loglevel_label);
    setupLoggingImp(stream, force_colorization, lvl);
}

void setupLogging(std::ostream &stream,
                  bool force_colorization,
                  lth_log::log_level const& lvl) {
    setupLoggingImp(stream, force_colorization, lvl);
}

}  // namespace Util
}  // namespace PCPClient

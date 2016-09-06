#include <cpp-pcp-client/util/logging.hpp>

#define LEATHERMAN_LOGGING_NAMESPACE "puppetlabs.pcp_client.configuration"
#include <leatherman/logging/logging.hpp>

#include <boost/log/core.hpp>
#include <boost/log/sinks/basic_sink_backend.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/attributes/scoped_attribute.hpp>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

#include <boost/smart_ptr/make_shared_object.hpp>

#include <map>
#include <fstream>
#include <cstdint>

namespace PCPClient {
namespace Util {

namespace lth_log = leatherman::logging;
namespace sinks = boost::log::sinks;
namespace expr = boost::log::expressions;
namespace attrs = boost::log::attributes;

static bool access_logger_enabled { false };

//
// PCP Access Logging - sink backend
//

BOOST_LOG_ATTRIBUTE_KEYWORD(access_outcome, "AccessOutcome", std::string)

class access_writer : public sinks::basic_sink_backend<sinks::synchronized_feeding>
{
  public:
    explicit access_writer(std::shared_ptr<std::ostream> sink_stream_ptr);
    void consume(boost::log::record_view const& rec);

  private:
    std::shared_ptr<std::ostream> _sink_stream_ptr;
};

access_writer::access_writer(std::shared_ptr<std::ostream> sink_stream_ptr)
    : _sink_stream_ptr {std::move(sink_stream_ptr)}
{
}

void access_writer::consume(boost::log::record_view const &rec)
{
    auto timestamp = boost::log::extract<boost::posix_time::ptime>("TimeStamp", rec);
    auto access_outcome = boost::log::extract<std::string>("AccessOutcome", rec);
    *_sink_stream_ptr << '[' << boost::gregorian::to_iso_extended_string(timestamp->date())
                      << ' ' << boost::posix_time::to_simple_string(timestamp->time_of_day())
                      << "] " << access_outcome << std::endl;
}

//
// PCP Access Logging - logger
//

void logAccess(std::string const& message, int line_num)
{
    if (access_logger_enabled) {
        boost::log::sources::severity_logger<lth_log::log_level> slg;
        static attrs::constant <std::string> namespace_attr{
                "puppetlabs.pcp_client.connector"};
        slg.add_attribute("AccessOutcome",
                          attrs::constant<std::string>(message));
        slg.add_attribute("Namespace", namespace_attr);
        slg.add_attribute("LineNum", attrs::constant<int>(line_num));

        // TODO(ale): make leatherman.logging's color_writer::consume ignore
        // empty messages (and don't stream 'message' below) or check the log
        // level; in alternative, allow configuring the color_writer sink
        // filter to avoid logging AccessOutcome messages (something like
        // `lth_sink->set_filter(!expr::has_attr(access_outcome));`) - note
        // that the on_message callback and the record log level are checked
        // by the logger, not by color_writer::consume. Otherwise, as it's now,
        // the access message will be logged if the configured log level > info.
        BOOST_LOG_SEV(slg, lth_log::log_level::info) << message;
    }
}

//
// setupLogging
//

static void setupLoggingImp(std::ostream& log_stream,
                            bool force_colorization,
                            lth_log::log_level const& lvl,
                            std::shared_ptr<std::ofstream> access_stream)
{
    // General Logging
    lth_log::setup_logging(log_stream);
    lth_log::set_level(lvl);

    if (force_colorization)
        lth_log::set_colorization(true);

    // PCP Access Logging
    if (access_stream) {
        access_logger_enabled = true;
        using sink_t = sinks::synchronous_sink<access_writer>;
        auto sink = boost::make_shared<sink_t>(std::move(access_stream));
        sink->set_filter(expr::has_attr(access_outcome));
        auto core = boost::log::core::get();
        core->add_sink(sink);

    } else {
        access_logger_enabled = false;
    }
}

void setupLogging(std::ostream &log_stream,
                  bool force_colorization,
                  std::string const& loglevel_label,
                  std::shared_ptr<std::ofstream> access_stream)
{
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
    setupLoggingImp(log_stream, force_colorization, lvl, std::move(access_stream));
}

void setupLogging(std::ostream &log_stream,
                  bool force_colorization,
                  lth_log::log_level const& lvl,
                  std::shared_ptr<std::ofstream> access_stream)
{
    setupLoggingImp(log_stream, force_colorization, lvl, std::move(access_stream));
}

}  // namespace Util
}  // namespace PCPClient

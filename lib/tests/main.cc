// Refer to https://github.com/philsquared/Catch/blob/master/docs/own-main.md
// for providing our own main function to Catch
#define CATCH_CONFIG_RUNNER

#include "test.hpp"

#define LEATHERMAN_LOGGING_NAMESPACE "puppetlabs.cpp_pcp_client.test"
#include <leatherman/logging/logging.hpp>

int main(int argc, char* const argv[]) {
    // set logging level to fatal
    leatherman::logging::setup_logging(std::cout);
    leatherman::logging::set_level(leatherman::logging::log_level::fatal);

    // configure the Catch session and start it
    Catch::Session test_session;
    test_session.applyCommandLine(argc, argv);

    // NOTE(ale): to list the reporters use:
    // test_session.configData().listReporters = true;

    // Reporters: "xml", "junit", "console", "compact"
    test_session.configData().reporterName = "console";

    // ShowDurations::Always, ::Never, ::DefaultForReporter
    test_session.configData().showDurations = Catch::ShowDurations::Always;

    return test_session.run();
}

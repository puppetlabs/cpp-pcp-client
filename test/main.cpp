#include "test.h"
#include "../src/log/log.h"

int main(int argc, char** argv) {
    // Configure boost log
    Cthun::Log::configure_logging(Cthun::Log::log_level::fatal, std::cout);

    // Run all tests
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

# allow include directives with absolute paths for cpp-pcp-client sources
set(BASEPATH "${CMAKE_CURRENT_LIST_DIR}/..")
include_directories("${BASEPATH}")
SET(SOURCE_DIR "${BASEPATH}/src")
include_directories("${SOURCE_DIR}")

set(SOURCES
    main.cc
    unit/connector/certs.cc
    unit/connector/client_metadata_test.cc
    unit/connector/connection_test.cc
    unit/connector/connector_base_test.cc
    unit/connector/mock_server.cc
    unit/connector/v1/connector_test.cc
    unit/connector/v2/connector_test.cc
    unit/protocol/v1/serialization_test.cc
    unit/protocol/v1/message_test.cc
    unit/protocol/v1/schemas_test.cc
    unit/protocol/v2/message_test.cc
    unit/validator/schema_test.cc
    unit/validator/validator_test.cc
)

include_directories(
    ${LEATHERMAN_CATCH_INCLUDE}
    ${VALIJSON_INCLUDE_DIRS}
)

if (WIN32)
    list(APPEND PLATFORM_LIBS Mswsock)
endif()

set(test_BIN cpp-pcp-client-unittests)
add_executable(${test_BIN} ${SOURCES})
# As a warning, don't put Boost.Log before libcpp-pcp-client. Boost.Log uses
# Boost.Asio in its syslog backend, and putting it before libcpp-pcp-client in
# linking causes segfaults as it picks up the wrong Asio symbols when using
# Boost.Asio directly in the test, and Boost.Log and libcpp-pcp-client are
# both dynamic libraries.
target_link_libraries(${test_BIN} libcpp-pcp-client ${LIBS} ${PLATFORM_LIBS})

add_custom_target(check
    "${EXECUTABLE_OUTPUT_PATH}/${test_BIN}"
    DEPENDS ${test_BIN}
    COMMENT "Executing unit tests..."
    VERBATIM
    SOURCES ${SOURCES}
)

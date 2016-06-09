# allow include directives with absolute paths for cpp-pcp-client sources
set(BASEPATH "${CMAKE_CURRENT_LIST_DIR}/..")
include_directories("${BASEPATH}")
SET(SOURCE_DIR "${BASEPATH}/src")
include_directories("${SOURCE_DIR}")

set(SOURCES
    main.cc
    unit/connector/client_metadata_test.cc
    unit/connector/connection_test.cc
    unit/connector/connector_test.cc
    unit/connector/certs.cc
    unit/protocol/serialization_test.cc
    unit/protocol/message_test.cc
    unit/protocol/schemas_test.cc
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

# On EL 4, we run into a linking error when trying to create libraries or
# executables that link in a static library with code using threads. As I
# described in https://gcc.gnu.org/ml/gcc-help/2015-08/msg00035.html, we get
# the error undefined reference to symbol '__tls_get_addr@@GLIBC_2.3'.
# Build mock_server as a separate shared library to avoid this error.

if (LEATHERMAN_SHARED)
    # Careful with the order, as done for libcpp-pcp-client
    list(APPEND MOCK_SERVER_LIBS
        ${Boost_REGEX_LIBRARY}
        ${Boost_THREAD_LIBRARY}
        ${Boost_SYSTEM_LIBRARY}
        ${LEATHERMAN_LIBRARIES})
else()
    list(APPEND MOCK_SERVER_LIBS
        ${LEATHERMAN_LIBRARIES}
        ${Boost_REGEX_LIBRARY}
        ${Boost_THREAD_LIBRARY}
        ${Boost_SYSTEM_LIBRARY})
endif()

# pcp-chunk provides cpp-pcp-clients functions needed by MockServer
add_library(pcp-chunks
    ${SOURCE_DIR}/protocol/chunks.cc
    ${SOURCE_DIR}/protocol/message.cc
    ${SOURCE_DIR}/protocol/serialization.cc
    ${SOURCE_DIR}/validator/schema.cc
    ${SOURCE_DIR}/validator/validator.cc)
target_link_libraries(pcp-chunks PRIVATE
    ${MOCK_SERVER_LIBS}
    ${PLATFORM_LIBS})
add_dependencies(pcp-chunks valijson)
# Add the libcpp-pcp-client export header
target_compile_definitions(pcp-chunks PRIVATE -Dlibcpp_pcp_client_EXPORTS)

add_library(mock-server unit/connector/mock_server.cc)
target_link_libraries(mock-server PRIVATE
    ${MOCK_SERVER_LIBS}
    ${OPENSSL_SSL_LIBRARY}
    ${OPENSSL_CRYPTO_LIBRARY}
    ${PLATFORM_LIBS}
    pcp-chunks)
add_dependencies(mock-server pcp-chunks websocketpp)

set(test_BIN cpp-pcp-client-unittests)
add_executable(${test_BIN} ${SOURCES})
# As a warning, don't put Boost.Log before libcpp-pcp-client. Boost.Log uses
# Boost.Asio in its syslog backend, and putting it before libcpp-pcp-client in
# linking causes segfaults as it picks up the wrong Asio symbols when using
# Boost.Asio directly in the test, and Boost.Log and libcpp-pcp-client are
# both dynamic libraries.
# Ensure mock-server comes after libcpp-pcp-client, so it picks up libraries
# compiled into libcpp-pcp-client. It comes after LIBS to avoid double
# definition of boost::system::system_category() on Windows.
target_link_libraries(${test_BIN} libcpp-pcp-client ${LIBS} pcp-chunks mock-server)

add_custom_target(check
    "${EXECUTABLE_OUTPUT_PATH}/${test_BIN}"
    DEPENDS ${test_BIN}
    COMMENT "Executing unit tests..."
    VERBATIM
    SOURCES ${SOURCES}
)

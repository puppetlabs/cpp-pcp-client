# allow include directives with absolute paths for cpp-pcp-client sources
set(BASEPATH "${CMAKE_CURRENT_LIST_DIR}/..")
include_directories("${BASEPATH}")
SET(SOURCE_DIR "${BASEPATH}/src")
include_directories("${SOURCE_DIR}")

set(SOURCES
    main.cc
    unit/connector/client_metadata_test.cc
    unit/connector/connection_test.cc
    unit/connector/certs.cc
    unit/protocol/serialization_test.cc
    unit/protocol/message_test.cc
    unit/protocol/schemas_test.cc
    unit/validator/schema_test.cc
    unit/validator/validator_test.cc
)

include_directories(
    ${LEATHERMAN_CATCH_INCLUDE}
)

if (WIN32)
    list(APPEND PLATFORM_LIBS Mswsock)
endif()

# On EL 4, we run into a linking error when trying to create libraries or
# executables that link in a static library with code using threads. As I
# described in https://gcc.gnu.org/ml/gcc-help/2015-08/msg00035.html, we get
# the error undefined reference to symbol '__tls_get_addr@@GLIBC_2.3'.
# Build mock_server as a separate shared library to avoid this error.
add_library(mock-server unit/connector/mock_server.cc)
target_link_libraries(mock-server PRIVATE
    ${Boost_THREAD_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${OPENSSL_SSL_LIBRARY}
    ${OPENSSL_CRYPTO_LIBRARY}
    ${PLATFORM_LIBS})

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
target_link_libraries(${test_BIN} libcpp-pcp-client ${LIBS} mock-server)

add_custom_target(check
    "${EXECUTABLE_OUTPUT_PATH}/${test_BIN}"
    DEPENDS ${test_BIN}
    COMMENT "Executing unit tests..."
    VERBATIM
    SOURCES ${SOURCES}
)

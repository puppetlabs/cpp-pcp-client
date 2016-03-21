include_directories(
    inc
    ${Boost_INCLUDE_DIRS}
    ${VALIJSON_INCLUDE_DIRS}
    ${WEBSOCKETPP_INCLUDE_DIRS}
    ${LEATHERMAN_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
)

set(SOURCES
    src/connector/client_metadata.cc
    src/connector/connection.cc
    src/connector/connector.cc
    src/connector/session_association.cc
    src/connector/timings.cc
    src/protocol/chunks.cc
    src/protocol/message.cc
    src/protocol/schemas.cc
    src/protocol/serialization.cc
    src/util/logging.cc
    src/validator/schema.cc
    src/validator/validator.cc
)

# TODO(ale): remove Thread dependency after LTH-81 changes
set(LIBS
    ${LEATHERMAN_LIBRARIES}
    ${Boost_LIBRARIES}
    ${OPENSSL_SSL_LIBRARY}
    ${OPENSSL_CRYPTO_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
)

if (WIN32)
    set(PLATFORM_LIBS Ws2_32)
elseif ("${CMAKE_SYSTEM_NAME}" MATCHES "SunOS")
    set(PLATFORM_LIBS socket nsl)
endif()

add_library(libcpp-pcp-client SHARED ${SOURCES})

# Explicit dependency on external projects to ensure they get
# extracted when building with multiple jobs
add_dependencies(libcpp-pcp-client websocketpp valijson)

target_link_libraries(libcpp-pcp-client PRIVATE ${LIBS} ${PLATFORM_LIBS})
set_target_properties(libcpp-pcp-client PROPERTIES PREFIX "" SUFFIX ".so" IMPORT_PREFIX "" IMPORT_SUFFIX ".so.a")

# Generate the export header
include(GenerateExportHeader)
generate_export_header(libcpp-pcp-client EXPORT_FILE_NAME "${CMAKE_CURRENT_LIST_DIR}/inc/cpp-pcp-client/export.h")

if (NOT APPLE)
    add_compiler_export_flags()
endif()
if (WIN32)
    add_definitions("-Dlibcpp_pcp_client_EXPORTS")
endif()

install(TARGETS libcpp-pcp-client
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)
install(DIRECTORY inc/cpp-pcp-client DESTINATION include)

add_subdirectory(tests)

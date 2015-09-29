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
    src/connector/timings.cc
    src/protocol/chunks.cc
    src/protocol/message.cc
    src/protocol/schemas.cc
    src/protocol/serialization.cc
    src/validator/schema.cc
    src/validator/validator.cc
)

set(LIBS
    ${LEATHERMAN_LIBRARIES}
    ${Boost_LIBRARIES}
    ${OPENSSL_SSL_LIBRARY}
    ${OPENSSL_CRYPTO_LIBRARY}
    ${PTHREADS}
)

if ("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
    set (PLATFORM_LIBRARIES rt)
elseif ("${CMAKE_SYSTEM_NAME}" MATCHES "SunOS")
    set (PLATFORM_LIBRARIES rt)
endif ()

if (WIN32)
    set(LIBCPP_PCP_CLIENT_INSTALL_DESTINATION bin)
    set (PLATFORM_LIBRARIES Ws2_32)
else()
    #   TODO: lib64 for certain operating systems?
    set(LIBCPP_PCP_CLIENT_INSTALL_DESTINATION lib)
endif()


add_library(libcpp-pcp-client-src OBJECT ${SOURCES})
set_target_properties(libcpp-pcp-client-src PROPERTIES POSITION_INDEPENDENT_CODE true)
add_library(libcpp-pcp-client SHARED $<TARGET_OBJECTS:libcpp-pcp-client-src>)
target_link_libraries(libcpp-pcp-client PRIVATE ${LIBS} ${PLATFORM_LIBRARIES})
set_target_properties(libcpp-pcp-client  PROPERTIES PREFIX "" SUFFIX ".so" IMPORT_PREFIX "" IMPORT_SUFFIX ".so.a")

install(TARGETS libcpp-pcp-client DESTINATION "${LIBCPP_PCP_CLIENT_INSTALL_DESTINATION}")
install(DIRECTORY inc/cpp-pcp-client DESTINATION include)

add_subdirectory(tests)

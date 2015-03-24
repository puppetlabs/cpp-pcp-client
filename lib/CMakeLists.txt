include_directories(
    inc
    ${Boost_INCLUDE_DIRS}
    ${RAPIDJSON_INCLUDE_DIRS}
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
    src/data_container/data_container.cc
    src/message/chunks.cc
    src/message/message.cc
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
endif ()

if (WIN32)
    set(LIBCTHUN_CLIENT_INSTALL_DESTINATION bin)
else()
    #   TODO: lib64 for certain operating systems?
    set(LIBCTHUN_CLIENT_INSTALL_DESTINATION lib)
endif()


add_library(libcthun-client-src OBJECT ${SOURCES})
set_target_properties(libcthun-client-src PROPERTIES POSITION_INDEPENDENT_CODE true)
add_library(libcthun-client SHARED $<TARGET_OBJECTS:libcthun-client-src>)
target_link_libraries(libcthun-client PRIVATE ${LIBS} ${PLATFORM_LIBRARIES})
set_target_properties(libcthun-client  PROPERTIES PREFIX "" SUFFIX ".so" IMPORT_PREFIX "" IMPORT_SUFFIX ".so.a")

install(TARGETS libcthun-client DESTINATION "${LIBCTHUN_CLIENT_INSTALL_DESTINATION}")

add_subdirectory(tests)

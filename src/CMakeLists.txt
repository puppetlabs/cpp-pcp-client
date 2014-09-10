# Set sources
SET(SOURCES
    log/log.cpp
    common/file_utils.cpp
    common/string_utils.cpp
    configuration.cpp
    endpoint.cpp
    connection.cpp
    connection_manager.cpp
)

SET(LIBS
    ${Boost_LIBRARIES}
    ${OPENSSL_SSL_LIBRARY}
    ${OPENSSL_CRYPTO_LIBRARY}
    ${PTHREADS}
)

ADD_LIBRARY(cthun-client ${SOURCES})
TARGET_LINK_LIBRARIES(cthun-client
    ${LIBS}
)

ADD_EXECUTABLE(cthun-client-bin main.cpp)
TARGET_LINK_LIBRARIES(cthun-client-bin cthun-client)
SET_TARGET_PROPERTIES(cthun-client-bin
    PROPERTIES
    OUTPUT_NAME cthun-client
    CLEAN_DIRECT_OUTPUT 1
)

INSTALL(TARGETS cthun-client-bin DESTINATION bin)

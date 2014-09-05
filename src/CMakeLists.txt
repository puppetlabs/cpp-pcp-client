# Set sources
SET(SOURCES
    log/log.cpp
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

ADD_LIBRARY(cthun_client ${SOURCES})
TARGET_LINK_LIBRARIES(cthun_client
    ${LIBS}
)

ADD_EXECUTABLE(cthun_client-bin main.cpp)
TARGET_LINK_LIBRARIES(cthun_client-bin cthun_client)
SET_TARGET_PROPERTIES(cthun_client-bin
    PROPERTIES
    OUTPUT_NAME cthun_client
    CLEAN_DIRECT_OUTPUT 1
)

INSTALL(TARGETS cthun_client-bin DESTINATION bin)

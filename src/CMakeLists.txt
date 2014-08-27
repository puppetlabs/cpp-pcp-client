# Set sources
SET(SOURCES
    test_client.cpp
    client.cpp
    connection_metadata.cpp
)

SET(LIBS
    ${Boost_LIBRARIES}
    ${OPENSSL_SSL_LIBRARY}
    ${OPENSSL_CRYPTO_LIBRARY}
    ${PTHREADS}
)

ADD_LIBRARY(cthun_ws_client ${SOURCES})
TARGET_LINK_LIBRARIES(cthun_ws_client
    ${LIBS}
)

ADD_EXECUTABLE(cthun_ws_client-bin main.cpp)
TARGET_LINK_LIBRARIES(cthun_ws_client-bin cthun_ws_client)
SET_TARGET_PROPERTIES(cthun_ws_client-bin
    PROPERTIES
    OUTPUT_NAME cthun_ws_client
    CLEAN_DIRECT_OUTPUT 1
)

INSTALL(TARGETS cthun_ws_client-bin DESTINATION bin)

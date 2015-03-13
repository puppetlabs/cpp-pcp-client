set(SOURCES
    connector/client_metadata.cpp
    connector/connection.cpp
    connector/connector.cpp
    connector/timings.cpp
    data_container/data_container.cpp
    message/chunks.cpp
    message/message.cpp
    validator/schema.cpp
    validator/validator.cpp
)

set(LIBS
    ${LEATHERMAN_LIBRARIES}
    ${Boost_LIBRARIES}
)

add_library(cthun-client ${SOURCES})
target_link_libraries(cthun-client PRIVATE ${LIBS})

install(TARGETS cthun-client DESTINATION lib)

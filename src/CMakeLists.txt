set(SOURCES
    connector/connection.cpp
    connector/connector.cpp
    connector/timings.cpp
    data_container/data_container.cpp
    message/message.cpp
    validator/schema.cpp
    validator/validator.cpp
)

set(LIBS
    ${Boost_LIBRARIES}
)

add_library(cthun-client ${SOURCES})
target_link_libraries(cthun-client ${LIBS})

target_link_libraries(cthun-client)

install(TARGETS cthun-client DESTINATION lib)

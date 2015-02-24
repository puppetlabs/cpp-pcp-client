set(SOURCES
    connector/timings.cpp
    # connector/connector.cpp
)

set(LIBS
    ${Boost_LIBRARIES}
)

add_library(cthun-client ${SOURCES})
target_link_libraries(cthun-client ${LIBS})

target_link_libraries(cthun-client)

install(TARGETS cthun-client DESTINATION lib)

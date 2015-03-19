# allow include directives with absolute paths for cthun-client sources
set(BASEPATH "${CMAKE_CURRENT_LIST_DIR}/..")
include_directories("${BASEPATH}")
SET(SOURCE_DIR "${BASEPATH}/src")
include_directories("${SOURCE_DIR}")
set(CATCH_DIRECTORY "${VENDOR_DIRECTORY}/catch")

set(SOURCES
    main.cpp

    unit/connector/client_metadata_test.cpp
    unit/connector/connection_test.cpp
    unit/connector/certs.cpp
    unit/data_container/data_container_test.cpp
    unit/message/serialization_test.cpp
    unit/message/message_test.cpp
    unit/validator/schema_test.cpp
    unit/validator/validator_test.cpp
)

set(test_BIN cthun-client-unittests)

find_package(Boost 1.54 REQUIRED
  COMPONENTS filesystem system date_time thread log regex random)

include_directories(
    ${CATCH_DIRECTORY}
    SYSTEM ${Boost_INCLUDE_DIRS}
)

add_executable(${test_BIN} ${SOURCES})
target_link_libraries(${test_BIN} libcthun-client)

set(LIBS
    ${Boost_LIBRARIES}
)

target_link_libraries(${test_BIN} ${LIBS})

add_custom_target(check
    "${MAINFOLDER}/bin/${test_BIN}"
    DEPENDS ${test_BIN}
    COMMENT "Executing unit tests..."
    VERBATIM
    SOURCES ${SOURCES}
)

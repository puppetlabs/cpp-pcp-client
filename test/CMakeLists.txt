set(SOURCES
    main.cpp
    connection_manager_test.cpp
    connection_test.cpp
    file_utils_test.cpp
)

SET (test_BIN ${PROJECT_NAME}-unittests)

include_directories(
    ${GMOCK_INCLUDE_DIRS}
)

ADD_EXECUTABLE(${test_BIN} ${SOURCES})
TARGET_LINK_LIBRARIES(
    ${test_BIN}
    cthun-client
    ${GMOCK_LIBRARIES}
)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lpthread -pthread")
endif()

ADD_CUSTOM_TARGET(check
    ALL
    "${MAINFOLDER}/bin/${test_BIN}"
    DEPENDS ${test_BIN}
    COMMENT "Executing unit tests..."
    VERBATIM
    SOURCES ${SOURCES})

ADD_CUSTOM_TARGET(test "${MAINFOLDER}/bin/${test_BIN}" DEPENDS ${test_BIN} COMMENT "Executing unit tests..." VERBATIM SOURCES ${test_SRCS})

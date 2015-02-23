# allow include directives with absolute paths for cthun-agent sources
set(BASEPATH "${CMAKE_CURRENT_LIST_DIR}/..")
include_directories("${BASEPATH}")
set(CATCH_DIRECTORY "${VENDOR_DIRECTORY}/catch")

set(SOURCES
    main.cpp

    unit/message/serialization_test.cpp
)

set(test_BIN cthun-client-unittests)

find_package(Boost 1.54 REQUIRED
  COMPONENTS filesystem system date_time thread log regex random)

include_directories(
    ${CATCH_DIRECTORY}
    SYSTEM ${Boost_INCLUDE_DIRS}
)

add_executable(${test_BIN} ${SOURCES})

set(LIBS
    ${Boost_LIBRARIES}
)

target_link_libraries(${test_BIN} ${LIBS})

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -Wno-reorder -Wno-unused-parameter -Wno-deprecated-register")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")

    # NB: -pthread is needed for fedora20 (cmake does not add the library, even
    # afer finding it)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -Wno-unused-parameter -lpthread -pthread")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
endif()

add_custom_target(check
    "${MAINFOLDER}/bin/${test_BIN}"
    DEPENDS ${test_BIN}
    COMMENT "Executing unit tests..."
    VERBATIM
    SOURCES ${SOURCES}
)

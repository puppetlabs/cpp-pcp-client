cmake_minimum_required(VERSION 2.8.12)
project(cthun-client)

# Project paths

set(MAINFOLDER ${PROJECT_SOURCE_DIR})
set(EXECUTABLE_OUTPUT_PATH "${MAINFOLDER}/bin")
set(LIBRARY_OUTPUT_PATH "${MAINFOLDER}/lib")
set(EXECUTABLE_OUTPUT_PATH "${MAINFOLDER}/bin")
set(VENDOR_DIRECTORY "${PROJECT_SOURCE_DIR}/vendor")
list(APPEND CMAKE_MODULE_PATH ${VENDOR_DIRECTORY})

if (not ${LEATHERLOGGING_PRIFIX})
  set(LEATHERMAN_LOGGING_PREFIX "puppetlabs.cthun-client.")
endif ()

if ("${CMAKE_PROJECT_NAME}" STREQUAL "${PROJECT_NAME}")
  set(CTHUN_CLIENT_TOPLEVEL TRUE)
else ()
   set(CTHUN_CLIENT_TOPLEVEL FALSE)
endif ()

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/vendor/leatherman/cmake")

# Set compiler flags

# Exclusions
# - Wsign-compare (valijson)
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -Werror -Wno-reorder -Wno-unused-parameter -Wno-deprecated-register -Wno-sign-compare -Wno-deprecated-declarations")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")

    # NB: -pthread is needed for fedora20 (cmake does not add the library, even
    # afer finding it)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare -lpthread -pthread")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
endif()

# Leatherman it up
include(options)

set(LEATHERMAN_USE_LOCALE TRUE)
set(LEATHERMAN_USE_NOWIDE TRUE)
set(LEATHERMAN_USE_CATCH TRUE)
set(LEATHERMAN_USE_LOGGING TRUE)
add_subdirectory("vendor/leatherman")

# Include vendor libraries and directories
include(${VENDOR_DIRECTORY}/websocketpp.cmake)
include(${VENDOR_DIRECTORY}/rapidjson.cmake)
include(${VENDOR_DIRECTORY}/valijson.cmake)

include_directories(
    ${RAPIDJSON_INCLUDE_DIRS}
    ${VALIJSON_INCLUDE_DIRS}
    ${WEBSOCKETPP_INCLUDE_DIRS}
    ${LEATHERMAN_INCLUDE_DIRS}
)

# Add src subdirectory

add_subdirectory(src)

# Add the test suite

if (CTHUN_CLIENT_TOPLEVEL)
    enable_testing()
    add_subdirectory(test)

    add_test(
      NAME "cthun-client\\ library\\ tests"
      COMMAND "${MAINFOLDER}/bin/cthun-client-unittests"

    )

    # Add cpplint target
    FILE (GLOB_RECURSE ALL_SOURCES src/*.cpp src/*.h test/*.cpp test/*.h)
    add_cpplint_files(${ALL_SOURCES})
    enable_cpplint()

endif()

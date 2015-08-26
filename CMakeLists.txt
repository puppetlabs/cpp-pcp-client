cmake_minimum_required(VERSION 2.8.12)
project(cpp-pcp-client)

# Project paths

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
set(VENDOR_DIRECTORY ${PROJECT_SOURCE_DIR}/vendor)
list(APPEND CMAKE_MODULE_PATH ${VENDOR_DIRECTORY})

if ("${CMAKE_PROJECT_NAME}" STREQUAL "${PROJECT_NAME}")
  set(CPP_PCP_CLIENT_TOPLEVEL TRUE)
else ()
   set(CPP_PCP_CLIENT_TOPLEVEL FALSE)
endif ()

if ("${CPP_PCP_CLIENT_LOGGING_PREFIX}" STREQUAL "")
  add_definitions(-DCPP_PCP_CLIENT_LOGGING_PREFIX="puppetlabs.cpp_pcp_client")
else ()
  add_definitions(-DCPP_PCP_CLIENT_LOGGING_PREFIX="${CPP_PCP_CLIENT_LOGGING_PREFIX}")
endif()

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/vendor/leatherman/cmake")

# Set the root path macro and expand related template
set(ROOT_PATH ${PROJECT_SOURCE_DIR})
configure_file(templates/root_path.hpp ${CMAKE_BINARY_DIR}/generated/root_path.hpp)
include_directories(${CMAKE_BINARY_DIR}/generated)

# Set compiler flags
include(cflags)
set(CPP_PCP_CLIENT_FLAGS "-Wno-deprecated-declarations -Wno-reorder -Wno-sign-compare")
set(CMAKE_CXX_FLAGS "${LEATHERMAN_CXX_FLAGS} ${CPP_PCP_CLIENT_FLAGS}")

# Leatherman it up
include(options)
option(BOOST_STATIC "Use Boost's static libraries" OFF)
set(LEATHERMAN_USE_LOCALE TRUE)
set(LEATHERMAN_USE_NOWIDE TRUE)
set(LEATHERMAN_USE_CATCH TRUE)
set(LEATHERMAN_USE_LOGGING TRUE)
set(LEATHERMAN_USE_RAPIDJSON TRUE)
set(LEATHERMAN_USE_JSON_CONTAINER TRUE)
set(LEATHERMAN_USE_UTIL TRUE)
leatherman_logging_line_numbers()
add_subdirectory("vendor/leatherman")

# Find libraries
find_package(Boost 1.54 REQUIRED
  COMPONENTS filesystem system date_time thread log regex random)

find_package(OpenSSL REQUIRED)

# prefer openssl from ports
if ("${CMAKE_SYSTEM_NAME}" MATCHES "Darwin")
    set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} /opt/local/lib)
    set(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} /opt/local/include)
else()
    set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} /usr/lib)
    set(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} /usr/include)
endif()

# Include vendor libraries and directories
include(${VENDOR_DIRECTORY}/websocketpp.cmake)
include(${VENDOR_DIRECTORY}/valijson.cmake)

link_directories(
    ${Boost_LIBRARY_DIRS}
    ${OPENSSL_INCLUDE_DIR}
)

# Add src subdirectory
add_subdirectory(lib)

# Add the test suite
if (CPP_PCP_CLIENT_TOPLEVEL)
    enable_testing()

    add_test(
      NAME "cpp-pcp-client\\ library\\ tests"
      COMMAND "${EXECUTABLE_OUTPUT_PATH}/cpp-pcp-client-unittests"
    )

    # Add cpplint target
    FILE (GLOB_RECURSE ALL_SOURCES lib/*.cc lib/*.hpp)
    add_cpplint_files(${ALL_SOURCES})
    enable_cpplint()

endif()

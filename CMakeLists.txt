cmake_minimum_required(VERSION 3.2.2)
project(cpp-pcp-client VERSION 1.7.0)

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

if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "Defaulting to a release build.")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

option(BUILD_SHARED_LIBS "Build shared library for cpp-pcp-client if on, otherwise build static libs" ON)

# Set the macro for leatherman's logging namespace
add_definitions(-DCPP_PCP_CLIENT_LOGGING_PREFIX="puppetlabs.cpp_pcp_client")

# Disable SSLv2 and SSLv3 (in Boost.Asio) for reals. Needed for some builds of OpenSSL.
add_definitions(-DOPENSSL_NO_SSL2 -DOPENSSL_NO_SSL3)

# Defined further options
option(DEV_LOG_RAW_MESSAGE "Enable logging serialized messages (development setting - avoid this on Windows)" OFF)

# Set the root path macro and expand related template
set(ROOT_PATH ${PROJECT_SOURCE_DIR})
configure_file(templates/root_path.hpp ${CMAKE_BINARY_DIR}/generated/root_path.hpp)
include_directories(${CMAKE_BINARY_DIR}/generated)

# Set RPATH if not installing to a system library directory
list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" INSTALL_IS_SYSTEM_DIR)
if ("${INSTALL_IS_SYSTEM_DIR}" STREQUAL "-1")
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
endif()

# Find libraries
find_package(Leatherman REQUIRED
  COMPONENTS locale nowide catch logging rapidjson json_container util)
find_package(Boost 1.54 REQUIRED
  COMPONENTS filesystem chrono system date_time thread log regex random)
find_package(OpenSSL REQUIRED)
# Needed for linking on AIX
find_package(Threads)

# Leatherman it up
include(options)
include(cflags)
leatherman_logging_line_numbers()

# Set compiler flags
set(CPP_PCP_CLIENT_FLAGS "-Wno-deprecated-declarations -Wno-reorder -Wno-sign-compare")
set(CMAKE_CXX_FLAGS "${LEATHERMAN_CXX_FLAGS} ${CPP_PCP_CLIENT_FLAGS}")
add_definitions(${LEATHERMAN_DEFINITIONS})

if (WIN32)
    link_libraries("-Wl,--nxcompat -Wl,--dynamicbase")
endif()

if(DEV_LOG_RAW_MESSAGE)
    add_definitions(-DDEV_LOG_RAW_MESSAGE)
endif()

# TODO(ale): enable i18n with add_definitions(-DLEATHERMAN_I18N) - PCP-257
# TODO(ale): enable translation with set(LEATHERMAN_LOCALES "...;...") and
# gettext_compile(${CMAKE_CURRENT_SOURCE_DIR}/locales share/locale)

# Configure i18n
file(GLOB_RECURSE PCP_CLIENT_SOURCES */src/*.cc */inc/*.hpp)
gettext_templates(${CMAKE_CURRENT_SOURCE_DIR}/locales ${PCP_CLIENT_SOURCES})

# Prefer openssl from ports
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

# Display a summary of the features
include(FeatureSummary)
feature_summary(WHAT ALL)

link_directories(
    ${Boost_LIBRARY_DIRS}
    ${OPENSSL_INCLUDE_DIR}
)

# Pull in helper macros for working with leatherman libraries
include(leatherman)

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

    add_cppcheck_dirs("${PROJECT_SOURCE_DIR}/lib")
    enable_cppcheck()
endif()

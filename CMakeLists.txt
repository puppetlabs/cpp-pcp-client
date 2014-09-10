# <Copyright Holder>. Copyright (C) <Copyright Year(s)>. <License>.

#
# Project Properties
#
CMAKE_MINIMUM_REQUIRED (VERSION 2.6.2)
PROJECT (cthun-client C CXX)
SET (APPLICATION_NAME "Cthun WebSocket Client")
SET (APPLICATION_CODENAME "${PROJECT_NAME}")
SET (APPLICATION_COPYRIGHT_YEARS "2014")
SET (APPLICATION_VERSION_MAJOR 0)
SET (APPLICATION_VERSION_MINOR 1)
SET (APPLICATION_VERSION_PATCH 0)
SET (APPLICATION_VERSION_TYPE SNAPSHOT)
SET (APPLICATION_VERSION_STRING
    "${APPLICATION_VERSION_MAJOR}.${APPLICATION_VERSION_MINOR}.${APPLICATION_VERSION_PATCH}-${APPLICATION_VERSION_TYPE}")
SET (APPLICATION_VENDOR_ID "com.puppetlabs")
SET (APPLICATION_VENDOR_NAME "Puppet Labs")
SET (APPLICATION_VENDOR_URL "puppetlabs.com")
SET (APPLICATION_ID "${APPLICATION_VENDOR_ID}.${PROJECT_NAME}")
SET (VENDOR_DIRECTORY "${PROJECT_SOURCE_DIR}/vendor")

#
# Debugging Options
#
SET (CMAKE_VERBOSE_MAKEFILE 0) # Use 1 for debugging, 0 for release
SET (CMAKE_BUILD_TYPE Debug)

#
# Project Output Paths
#
SET (MAINFOLDER ${PROJECT_SOURCE_DIR})
SET (EXECUTABLE_OUTPUT_PATH "${MAINFOLDER}/bin")
SET (LIBRARY_OUTPUT_PATH "${MAINFOLDER}/lib")

#
# Project Search Paths
#
LIST (APPEND CMAKE_PREFIX_PATH "${MAINFOLDER}")
SET (CMAKE_MODULE_PATH "${MAINFOLDER}/vendor")
INCLUDE_DIRECTORIES("${MAINFOLDER}/src")
INCLUDE_DIRECTORIES("${MAINFOLDER}/include")

#
# Dependency Packages
#
FIND_PACKAGE(Boost 1.55 REQUIRED COMPONENTS program_options system random log
             thread date_time regex)
if (NOT Boost_FOUND)
    MESSAGE (FATAL_ERROR "Boost is required. Please install it before using ${APPLICATION_NAME}")
endif ()
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})

FIND_PACKAGE(OpenSSL REQUIRED)

FIND_PACKAGE(Threads REQUIRED)

#
# Refer vendor libraries
#
# TODO(ale): include horsewhisperer
INCLUDE(${VENDOR_DIRECTORY}/websocketpp.cmake)
include(${VENDOR_DIRECTORY}/gmock.cmake)

#
# Include vendor directories
#
INCLUDE_DIRECTORIES(
    ${WEBSOCKETPP_INCLUDE_DIRS}
    ${GMOCK_INCLUDE_DIRS}
)

#
# Configure Files
#
FILE (GLOB_RECURSE CONFIGINPUTS1 include/*.in.h.cmake)
FILE (GLOB_RECURSE CONFIGINPUTS2 include/*.h.in.cmake)
FILE (GLOB_RECURSE CONFIGINPUTS3 src/*.in.h.cmake)
FILE (GLOB_RECURSE CONFIGINPUTS4 src/*.h.in.cmake)
FILE (GLOB_RECURSE CONFIGINPUTS5 test/*.in.h.cmake)
FILE (GLOB_RECURSE CONFIGINPUTS6 test/*.h.in.cmake)
LIST (APPEND CONFIGINPUTS "${CONFIGINPUTS1}")
LIST (APPEND CONFIGINPUTS "${CONFIGINPUTS2}")
LIST (APPEND CONFIGINPUTS "${CONFIGINPUTS3}")
LIST (APPEND CONFIGINPUTS "${CONFIGINPUTS4}")
LIST (APPEND CONFIGINPUTS "${CONFIGINPUTS5}")
LIST (APPEND CONFIGINPUTS "${CONFIGINPUTS6}")
FOREACH (CONFIGINPUT ${CONFIGINPUTS})
    STRING (REPLACE ".in.h.cmake" ".h" CONFIGOUTPUT1 "${CONFIGINPUT}")
    STRING (REPLACE ".h.in.cmake" ".h" CONFIGOUTPUT2 "${CONFIGOUTPUT1}")
    CONFIGURE_FILE ("${CONFIGINPUT}" "${CONFIGOUTPUT2}")
ENDFOREACH (CONFIGINPUT ${CONFIGINPUTS})

#
# Set specfic compile flags
#
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -Werror -Wno-unused-parameter -Wno-missing-field-initializers -Wno-ignored-qualifiers")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -Werror -Wno-unused-parameter -Wno-missing-field-initializers -Wno-unused-local-typedefs -Wno-ignored-qualifiers")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
endif()

#
# Add Build Targets
#
ADD_SUBDIRECTORY(src)

#
# Testing
#
ENABLE_TESTING()
ADD_SUBDIRECTORY(test)

#
# Add cpplint target
#
include(FindPythonInterp)
if (NOT PYTHONINTERP_FOUND)
    message(STATUS "Python not found; 'cpplint' target will not be available")
else()
    set(CPPLINT_FILTER
        "-build/include"
        "-legal/copyright"
        "-runtime/references" # Not sure about this religion
        "-build/header_guard"
        "-readability/streams" # we are using them sparingly
        "-whitespace/line_length" # we'll get there
        "-build/c++11"
        "-whitespace/indent"
    )

    FILE (GLOB_RECURSE ALL_SOURCES src/*.cpp src/*.h test/*.cpp test/*.h)

    set(CPPLINT_PATH "${MAINFOLDER}/ext/cpplint.py")

    set(CPPLINT_ARGS "")
    if (CPPLINT_FILTER)
        string(REPLACE ";" "," CPPLINT_FILTER "${CPPLINT_FILTER}")
        set(CPPLINT_ARGS "${CPPLINT_ARGS}--filter=${CPPLINT_FILTER}")
    endif()
    if (MSVC)
        set(CPPLINT_ARGS "${CPPLINT_ARGS} --output=vs7")
    endif()

    ADD_CUSTOM_TARGET(cpplint
        COMMAND ${PYTHON_EXECUTABLE} ${CPPLINT_PATH} ${CPPLINT_ARGS} ${ALL_SOURCES}
        VERBATIM
    )

endif()

#
# Add Install Targets
#
IF (EXISTS "${MAINFOLDER}/include/${PROJECT_NAME}" AND IS_DIRECTORY "${MAINFOLDER}/include/${PROJECT_NAME}")
    INSTALL(DIRECTORY "${MAINFOLDER}/include/${PROJECT_NAME}" DESTINATION "include")
ENDIF (EXISTS "${MAINFOLDER}/include/${PROJECT_NAME}" AND IS_DIRECTORY "${MAINFOLDER}/include/${PROJECT_NAME}")

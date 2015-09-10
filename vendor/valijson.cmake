cmake_minimum_required(VERSION 2.8.12)
include(ExternalProject)


# zip sourced from https://github.com/tristanpenman/valijson/archive/master.zip

# Add an external project to unpack valijson
externalproject_add(
    valijson
    PREFIX "${PROJECT_BINARY_DIR}"
    URL "file://${VENDOR_DIRECTORY}/valijson-600aeb9.zip"
    URL_MD5 "ce53330afc51281e39d6613041448133"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    BUILD_IN_SOURCE 1
    INSTALL_COMMAND ""
)

# Set some useful variables based on the source directory
externalproject_get_property(valijson SOURCE_DIR)
set(VALIJSON_INCLUDE_DIRS "${SOURCE_DIR}/include")

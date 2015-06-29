INCLUDE(ExternalProject)

# Add an external project to build websocketpp
EXTERNALPROJECT_ADD(
    websocketpp
    PREFIX "${PROJECT_BINARY_DIR}"
    URL "file://${VENDOR_DIRECTORY}/websocketpp-0.6.0.tar.gz"
    URL_MD5 "5a485884c01f881aafbf1e055d851b82"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    BUILD_IN_SOURCE 1
    INSTALL_COMMAND ""
)

EXTERNALPROJECT_GET_PROPERTY(websocketpp SOURCE_DIR)
SET(WEBSOCKETPP_INCLUDE_DIRS "${SOURCE_DIR}")

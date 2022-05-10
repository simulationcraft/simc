# Use Cpack for packaging Simc
# This can be used to package eg. a nightly release

# Parse version from config file
file(READ "${PROJECT_SOURCE_DIR}/engine/config.hpp" SC_CONFIG_FILE_CONTENT)
string(REGEX MATCH "SC_MAJOR_VERSION \"([0-9]*)\"" SC_MAJOR_VERSION_MATCH ${SC_CONFIG_FILE_CONTENT})
if (SC_MAJOR_VERSION_MATCH)
  message(VERBOSE "Simc major version: ${CMAKE_MATCH_1}")
  set(CPACK_PACKAGE_VERSION_MAJOR ${CMAKE_MATCH_1})
endif()
string(REGEX MATCH "SC_MINOR_VERSION \"([0-9]*)\"" SC_MINOR_VERSION_MATCH ${SC_CONFIG_FILE_CONTENT})
if (SC_MINOR_VERSION_MATCH)
  message(VERBOSE "Simc minor version: ${CMAKE_MATCH_1}")
  set(CPACK_PACKAGE_VERSION_MINOR ${CMAKE_MATCH_1})
endif()
set(CPACK_PACKAGE_VERSION_PATCH ${GIT_COMMIT_HASH})
message(VERBOSE "CPACK_PACKAGE_VERSION_PATCH: ${CPACK_PACKAGE_VERSION_PATCH}")

# We need to manually compose CPACK_PACKAGE_VERSION since cmake does no longer like the non-digit git hash in the patch part, see
# https://gitlab.kitware.com/cmake/cmake/-/issues/19310
set(CPACK_PACKAGE_VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH})
message(VERBOSE "CPACK_PACKAGE_VERSION: ${CPACK_PACKAGE_VERSION}")


# Global options
set(CPACK_PACKAGE_VENDOR "SimulationCraft")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${PROJECT_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://www.simulationcraft.org/")
set(CPACK_PACKAGE_ICON "${PROJECT_SOURCE_DIR}/qt/icon/Simcraft2.ico")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${PROJECT_SOURCE_DIR}/README.md")

if(WIN32)
  set(CPACK_GENERATOR "7Z")
endif()

include(CPack)
# Projects Settings   
cmake_minimum_required (VERSION 3.10...3.21)

# allow enabling lto from the command line
if(POLICY CMP0069)
    cmake_policy(SET CMP0069 NEW)
endif()

project(simc LANGUAGES CXX)

# switch on/off different build targets
option(BUILD_GUI "Build the Qt gui along with cli binary" ON)

# disable various features that may be anvailable or unneeded
option(SC_NO_THREADING "Disable all dependencies on pthreads" OFF)
option(SC_NO_NETWORKING "Disable all networking related stuff." OFF)

set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ standard to conform to")
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS OFF)

# Require /permissive- when building with Visual Studio
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/permissive->)

# enable colored output with ninja build system
if(${CMAKE_GENERATOR} STREQUAL "Ninja")
    if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
        add_compile_options (-fdiagnostics-color=always)
    elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
        add_compile_options (-fcolor-diagnostics)
    endif()
endif()

# Default Blizzard Community Platform API key
if (DEFINED SC_DEFAULT_APIKEY)
    add_compile_options(-DSC_DEFAULT_APIKEY="${SC_DEFAULT_APIKEY}")
endif()

# output a compile_commands.json used for editor tooling
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Install paths
if (UNIX)
    set(SIMC_INSTALL_BIN "bin")
    set(SIMC_INSTALL_SHARED "share/SimulationCraft/SimulationCraft")
else() # Otherwise, install everything into a flat directory
    set(SIMC_INSTALL_BIN ".")
    set(SIMC_INSTALL_SHARED ".")
endif()

# link in all activated targets
add_subdirectory(engine)
if (BUILD_GUI)
  add_subdirectory(qt)
endif()

# enable warnings
if (NOT MSVC)
    target_compile_options(engine PUBLIC -Wall -Wextra)
endif()


# 'simc' command line interface target
include(source_files/cmake_engine_main.txt)
string(REGEX REPLACE "([^;]+)" "engine/\\1" source_files "${source_files}")
add_executable(simc ${source_files})
target_link_libraries(simc engine)
install(TARGETS simc DESTINATION ${SIMC_INSTALL_BIN})
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/profiles/ DESTINATION ${SIMC_INSTALL_SHARED}/profiles
    FILES_MATCHING PATTERN "*.simc" )
install(FILES 
${CMAKE_CURRENT_SOURCE_DIR}/README.md
${CMAKE_CURRENT_SOURCE_DIR}/CONTRIBUTING.md
${CMAKE_CURRENT_SOURCE_DIR}/COPYING
${CMAKE_CURRENT_SOURCE_DIR}/LICENSE
${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.BOOST
${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.BSD
${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.BSD2
${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.LGPL
${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.MIT
${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.UNLICENSE
DESTINATION ${SIMC_INSTALL_SHARED})
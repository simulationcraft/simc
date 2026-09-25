# Projects Settings
cmake_minimum_required (VERSION 3.13...3.22)

# Set OSX minimum version parity with Qt project
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.15)

project(simc LANGUAGES CXX)

# switch on/off different build targets
option(BUILD_GUI "Build the Qt gui along with cli binary" ON)

# disable various features that may be anvailable or unneeded
option(SC_NO_THREADING "Disable all dependencies on pthreads" OFF)
option(SC_NO_NETWORKING "Disable all networking related stuff." OFF)

# Install everything into a flat folder structure by default for packaging on windows
option(SC_USE_FLAT_INSTALL "Install files into a flat folder structure" ${WIN32})

option(SC_EVENT_QUEUE_DEBUG "Enable Event Queue Debug Info" OFF)

option(SC_LTO          "Enable Link-Time Optimization"         OFF)
option(SC_MARCH_NATIVE "Tune for the host CPU (-march=native)" OFF)
option(SC_PGO_GENERATE "Instrument for PGO profile collection" OFF)
set(SC_PGO_USE "" CACHE STRING "Absolute path to .profdata for PGO-optimized build")

if(SC_PGO_GENERATE AND SC_PGO_USE)
  message(FATAL_ERROR "SC_PGO_GENERATE and SC_PGO_USE are mutually exclusive.")
endif()

set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ standard to conform to")
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS OFF)

# enable colored output with ninja build system
if(${CMAKE_GENERATOR} STREQUAL "Ninja")
    if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
        add_compile_options (-fdiagnostics-color=always)
    elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
        add_compile_options (-fcolor-diagnostics)
    endif()
endif()

# Install paths
if (SC_USE_FLAT_INSTALL)
  set(SIMC_INSTALL_BIN ".")
  set(SIMC_INSTALL_SHARED ".")
else()
  set(SIMC_INSTALL_BIN "bin")
  set(SIMC_INSTALL_SHARED "share/SimulationCraft/SimulationCraft")
endif()

# output a compile_commands.json used for editor tooling
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

function(sc_common_compiler_options target)
  target_compile_options(${target} PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/permissive->
    $<$<CXX_COMPILER_ID:MSVC>:/W3>
    $<$<CXX_COMPILER_ID:MSVC>:/bigobj> # required for dbc item data object files
    $<$<CXX_COMPILER_ID:MSVC>:/utf-8> # required for fmt 11.2
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
  )
  if(NOT MSVC)
    set(_sc_extra_flags "")
    if(SC_LTO)
      list(APPEND _sc_extra_flags
        $<$<CXX_COMPILER_ID:Clang,AppleClang>:-flto=thin>
        $<$<CXX_COMPILER_ID:GNU>:-flto>
      )
    endif()
    if(SC_MARCH_NATIVE)
      list(APPEND _sc_extra_flags -march=native)
    endif()
    if(SC_PGO_GENERATE)
      list(APPEND _sc_extra_flags
        $<$<CXX_COMPILER_ID:Clang,AppleClang>:-fprofile-instr-generate>
        $<$<CXX_COMPILER_ID:GNU>:-fprofile-generate>
      )
    endif()
    if(SC_PGO_USE)
      list(APPEND _sc_extra_flags
        $<$<CXX_COMPILER_ID:Clang,AppleClang>:-fprofile-instr-use=${SC_PGO_USE}>
        $<$<CXX_COMPILER_ID:GNU>:-fprofile-use=${SC_PGO_USE}>
      )
    endif()
    if(_sc_extra_flags)
      target_compile_options(${target} PRIVATE ${_sc_extra_flags})
      target_link_options(${target} PRIVATE ${_sc_extra_flags})
    endif()
  endif()
endfunction()


### Git Hash ###
# Get the current working branch
execute_process(
    COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
# Get the latest abbreviated commit hash of the working branch
execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# link in all activated targets
add_subdirectory(engine)
if (BUILD_GUI)
  add_subdirectory(qt)
endif()

# 'simc' command line interface target
add_executable(simc engine/sc_main.cpp)
target_link_libraries(simc engine)
sc_common_compiler_options(simc)

install(TARGETS simc DESTINATION ${SIMC_INSTALL_BIN})

install(DIRECTORY profiles/ DESTINATION ${SIMC_INSTALL_SHARED}/profiles
    FILES_MATCHING PATTERN "*.simc" )
install(FILES
  README.md
  CONTRIBUTING.md
  COPYING
  LICENSE
  LICENSE.BOOST
  LICENSE.BSD
  LICENSE.BSD2
  LICENSE.LGPL
  LICENSE.MIT
  LICENSE.UNLICENSE
DESTINATION ${SIMC_INSTALL_SHARED})

include(${PROJECT_SOURCE_DIR}/cmake/package.cmake)

option(BUILD_TESTING "Build tests" ON)
if (BUILD_TESTING)
  include(CTest)
  enable_testing()
  add_subdirectory(tests)
endif()

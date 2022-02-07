# Projects Settings   
cmake_minimum_required (VERSION 3.10...3.22)

project(simc LANGUAGES CXX)

# switch on/off different build targets
option(BUILD_GUI "Build the Qt gui along with cli binary" ON)

# disable various features that may be anvailable or unneeded
option(SC_NO_THREADING "Disable all dependencies on pthreads" OFF)
option(SC_NO_NETWORKING "Disable all networking related stuff." OFF)

# Install everything into a flat folder structure by default for packaging on windows
if (WINDOWS)
  option(SC_USE_FLAT_INSTALL "Install files into a flat folder structure" YES)
else()
  option(SC_USE_FLAT_INSTALL "Install files into a flat folder structure" NO)
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

# helper function to enable warnings on targets
function(sc_enable_warnings target)
  target_compile_options(${target} PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
  )
endfunction()

function(sc_common_compiler_options target)
  sc_enable_warnings(${target})
  
  # Require /permissive- when building with Visual Studio
  target_compile_options(${target} PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/permissive->
  )
endfunction()


# link in all activated targets
add_subdirectory(engine)
if (BUILD_GUI)
  add_subdirectory(qt)
endif()

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
# Projects Settings   
cmake_minimum_required (VERSION 3.1)
project(simc)

# switch on/off different build targets
option(BUILD_GUI "Build the Qt gui along with cli binary" ON)

# disable various features that may be anvailable or unneeded
option(SC_NO_THREADING "Disable all dependencies on pthreads" OFF)
option(SC_NO_NETWORKING "Disable all networking related stuff." OFF)

set(CMAKE_CXX_STANDARD 14)

# enable colored output with ninja build system
if(${CMAKE_GENERATOR} STREQUAL "Ninja")
    if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
        add_compile_options (-fdiagnostics-color=always)
    elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
        add_compile_options (-fcolor-diagnostics)
    endif()
endif()

# output a compile_commands.json used for editor tooling
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# link in all activated targets
add_subdirectory(engine)
if (BUILD_GUI)
  add_subdirectory(qt)
endif()

# enable warnings
if (MSVC)
    target_compile_options(engine PUBLIC /W4)
else()
    target_compile_options(engine PUBLIC -Wall -Wextra)
endif()

# 'simc' command line interface target
include(source_files/cmake_engine_main.txt)
string(REGEX REPLACE "([^;]+)" "engine/\\1" source_files "${source_files}")
add_executable(simc ${source_files})
target_link_libraries(simc engine)


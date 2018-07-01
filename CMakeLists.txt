# Projects Settings   
cmake_minimum_required (VERSION 3.1)
project(simc)

# all options to individualize the build
option(BUILD_GUI "Build the Qt gui along with cli binary" ON)

set(CMAKE_CXX_STANDARD 14)

# enable colored output with ninja build system
add_compile_options(-fcolor-diagnostics)

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
add_executable(simc engine/sc_main.cpp)
target_link_libraries(simc engine)
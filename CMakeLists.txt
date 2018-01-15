# Projects Settings   
cmake_minimum_required (VERSION 3.1)
project(simc)


set (CMAKE_CXX_STANDARD 11)

add_subdirectory(engine)
add_subdirectory(qt)

# 'simc' command line interface target
add_executable(simc engine/sc_main.cpp)
target_link_libraries(simc engine)
# Projects Settings   
cmake_minimum_required (VERSION 3.1)
project(simc)


set (CMAKE_CXX_STANDARD 11)

add_subdirectory(engine)
add_subdirectory(qt)

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
add_definitions("-DSC_GIT_REV=\"${GIT_COMMIT_HASH}\"")
add_definitions("-DSC_GIT_BRANCH=\"${GIT_BRANCH}\"")

# 'simc' command line interface target
add_executable(simc engine/sc_main.cpp)
target_link_libraries(simc engine)
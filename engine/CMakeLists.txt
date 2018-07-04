##################
# 'engine' library
##################

# Define engine library
include(../source_files/cmake_engine.txt)
add_library(engine ${source_files})
target_include_directories(engine PUBLIC .)

# Detect pthreads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(engine Threads::Threads)

# Detect OpenSSL
if(APPLE)
    find_library(MACOS_SECURITY Security)
    find_library(MACOS_CORE_FOUNDATION CoreFoundation)
    target_link_libraries(engine ${MACOS_SECURITY} ${MACOS_CORE_FOUNDATION})
    message(STATUS "Using macOS native SSL library")
else()
    find_package(OpenSSL QUIET)
    if(${OPENSSL_FOUND})
        message(STATUS "Using OpenSSL")
        target_compile_definitions(engine PUBLIC SC_USE_OPENSSL)
    else()
        message(WARNING "No SSL library available. HTTPS access not available.")
    endif()
endif()

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
target_compile_definitions(engine PUBLIC "SC_GIT_REV=\"${GIT_COMMIT_HASH}\"" "SC_GIT_BRANCH=\"${GIT_BRANCH}\"")
add_custom_target(update_git_info ALL DEPENDS util/git_info.cpp)
add_dependencies(engine update_git_info)

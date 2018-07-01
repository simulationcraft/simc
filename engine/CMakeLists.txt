##################
# 'engine' library
##################

# PTHREAD settings
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Add all .cpp files recursively
file(GLOB_RECURSE engine_source *.cpp)

# Define engine library
add_library(engine ${engine_source})
target_link_libraries(engine Threads::Threads)
target_include_directories(engine PUBLIC ./)
target_include_directories(engine PUBLIC ./util/)

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

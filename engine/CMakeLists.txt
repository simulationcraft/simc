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
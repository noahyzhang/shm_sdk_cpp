cmake_minimum_required(VERSION 2.8)

project(thread_mem_shm_sdk_cpp)

include_directories(
    ./include
)

add_executable(read_process
    examples/sample/read_process.cpp
)

add_executable(write_process
    examples/sample/write_process.cpp
)

add_executable(performance_test
    examples/performance_test/api_performance.cpp
)

target_link_libraries(read_process
    pthread
)

target_link_libraries(write_process
    pthread
)

target_link_libraries(performance_test
    pthread
)


cmake_minimum_required(VERSION 3.20)
project(BDI_Phase1_Performance C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O3 -march=native -pthread")

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Ring buffer library
add_library(phase1_rings STATIC
    rings/spsc_ring.c
    rings/mpsc_ring.c
)

# Fiber library
add_library(phase1_fibers STATIC
    fibers/fiber.c
    fibers/fiber_scheduler.c
)

# Arena library
add_library(phase1_arena STATIC
    arena/shared_arena.c
)

# IPC library
add_library(phase1_ipc STATIC
    ipc/descriptor.c
    ipc/zero_copy.c
    ipc/graph_call.c
)

# Integration library
add_library(phase1_integration STATIC
    integration/phase1_init.c
)
target_link_libraries(phase1_integration
    phase1_rings
    phase1_fibers
    phase1_arena
    phase1_ipc
)

# Tests
enable_testing()

add_executable(test_spsc_ring tests/test_spsc_ring.c)
target_link_libraries(test_spsc_ring phase1_rings pthread)
add_test(NAME test_spsc_ring COMMAND test_spsc_ring)

add_executable(test_fiber tests/test_fiber.c)
target_link_libraries(test_fiber phase1_fibers)
add_test(NAME test_fiber COMMAND test_fiber)

# Benchmarks
add_executable(bench_rings bench/bench_rings.c)
target_link_libraries(bench_rings phase1_rings pthread)

# Install
install(TARGETS phase1_rings phase1_fibers phase1_arena phase1_ipc phase1_integration
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

install(DIRECTORY rings/ fibers/ arena/ ipc/ integration/
        DESTINATION include/phase1
        FILES_MATCHING PATTERN "*.h")

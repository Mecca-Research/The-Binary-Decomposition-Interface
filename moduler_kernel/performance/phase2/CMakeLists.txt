
cmake_minimum_required(VERSION 3.10)
project(phase2_memory_scheduling C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O3 -march=native")

# Options
option(PHASE2_NUMA "Enable NUMA optimization" ON)
option(PHASE2_HUGE_PAGES "Enable huge pages" ON)
option(PHASE2_PCID "Enable PCID/ASID" ON)
option(PHASE2_TICKLESS "Enable tickless operation" ON)
option(PHASE2_BUILD_TESTS "Build tests" ON)
option(PHASE2_BUILD_BENCH "Build benchmarks" ON)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/numa
    ${CMAKE_CURRENT_SOURCE_DIR}/prefetch
    ${CMAKE_CURRENT_SOURCE_DIR}/scheduler
    ${CMAKE_CURRENT_SOURCE_DIR}/integration
    ${CMAKE_CURRENT_SOURCE_DIR}/../phase1
)

# NUMA subsystem
set(NUMA_SOURCES
    numa/numa_topology.c
    numa/per_cpu_arena.c
    numa/attention.c
)

# Prefetch subsystem
set(PREFETCH_SOURCES
    prefetch/huge_pages.c
    prefetch/pcid.c
)

# Scheduler subsystem
set(SCHEDULER_SOURCES
    scheduler/timer_wheel.c
)

# Integration
set(INTEGRATION_SOURCES
    integration/phase2_init.c
)

# All sources
set(PHASE2_SOURCES
    ${NUMA_SOURCES}
    ${PREFETCH_SOURCES}
    ${SCHEDULER_SOURCES}
    ${INTEGRATION_SOURCES}
)

# Phase 2 library
add_library(phase2 STATIC ${PHASE2_SOURCES})
target_link_libraries(phase2 pthread m)

# Tests
if(PHASE2_BUILD_TESTS)
    enable_testing()
    
    add_executable(test_numa_topology tests/test_numa_topology.c)
    target_link_libraries(test_numa_topology phase2)
    add_test(NAME test_numa_topology COMMAND test_numa_topology)
    
    add_executable(test_per_cpu_arena tests/test_per_cpu_arena.c)
    target_link_libraries(test_per_cpu_arena phase2)
    add_test(NAME test_per_cpu_arena COMMAND test_per_cpu_arena)
    
    add_executable(test_attention tests/test_attention.c)
    target_link_libraries(test_attention phase2)
    add_test(NAME test_attention COMMAND test_attention)
    
    add_executable(test_huge_pages tests/test_huge_pages.c)
    target_link_libraries(test_huge_pages phase2)
    add_test(NAME test_huge_pages COMMAND test_huge_pages)
    
    add_executable(test_pcid tests/test_pcid.c)
    target_link_libraries(test_pcid phase2)
    add_test(NAME test_pcid COMMAND test_pcid)
    
    add_executable(test_timer_wheel tests/test_timer_wheel.c)
    target_link_libraries(test_timer_wheel phase2)
    add_test(NAME test_timer_wheel COMMAND test_timer_wheel)
    
    add_executable(test_integration tests/test_integration.c)
    target_link_libraries(test_integration phase2)
    add_test(NAME test_integration COMMAND test_integration)
endif()

# Benchmarks
if(PHASE2_BUILD_BENCH)
    add_executable(bench_phase2 bench/bench_phase2.c)
    target_link_libraries(bench_phase2 phase2)
    
    add_executable(bench_numa bench/bench_numa.c)
    target_link_libraries(bench_numa phase2)
    
    add_executable(bench_prefetch bench/bench_prefetch.c)
    target_link_libraries(bench_prefetch phase2)
    
    add_executable(bench_scheduler bench/bench_scheduler.c)
    target_link_libraries(bench_scheduler phase2)
endif()

# Install
install(TARGETS phase2 DESTINATION lib)
install(DIRECTORY numa/ DESTINATION include/phase2/numa FILES_MATCHING PATTERN "*.h")
install(DIRECTORY prefetch/ DESTINATION include/phase2/prefetch FILES_MATCHING PATTERN "*.h")
install(DIRECTORY scheduler/ DESTINATION include/phase2/scheduler FILES_MATCHING PATTERN "*.h")
install(DIRECTORY integration/ DESTINATION include/phase2/integration FILES_MATCHING PATTERN "*.h")

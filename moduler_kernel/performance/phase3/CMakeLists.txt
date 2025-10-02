cmake_minimum_required(VERSION 3.10)
project(BDI_Phase3 C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O3 -march=native")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-strict-aliasing -fPIC")

# Include Phase 1 and Phase 2
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../phase1
    ${CMAKE_CURRENT_SOURCE_DIR}/../phase2
)

# NVMe subsystem
set(NVME_SOURCES
    nvme/nvme_device.c
    nvme/nvme_queue.c
    nvme/nvme_io.c
)

# Network subsystem (stubs for now)
set(NET_SOURCES
    net/net_device.c
    net/pmd.c
    net/mbuf.c
)

# GPU subsystem (stubs for now)
set(GPU_SOURCES
    gpu/gpu_device.c
    gpu/gpu_memory.c
    gpu/gpu_kernel.c
)

# Integration
set(INTEGRATION_SOURCES
    integration/phase3_init.c
)

# Build Phase 3 library
add_library(bdi_phase3 STATIC
    ${NVME_SOURCES}
    ${NET_SOURCES}
    ${GPU_SOURCES}
    ${INTEGRATION_SOURCES}
)

# Tests
enable_testing()

add_executable(test_nvme_queue tests/test_nvme_queue.c)
target_link_libraries(test_nvme_queue bdi_phase3 pthread)
add_test(NAME test_nvme_queue COMMAND test_nvme_queue)

# Benchmarks
add_executable(bench_phase3 bench/bench_phase3.c)
target_link_libraries(bench_phase3 bdi_phase3 pthread m)

# Install
install(TARGETS bdi_phase3 DESTINATION lib)
install(DIRECTORY nvme/ DESTINATION include/bdi/phase3/nvme FILES_MATCHING PATTERN "*.h")
install(DIRECTORY net/ DESTINATION include/bdi/phase3/net FILES_MATCHING PATTERN "*.h")
install(DIRECTORY gpu/ DESTINATION include/bdi/phase3/gpu FILES_MATCHING PATTERN "*.h")

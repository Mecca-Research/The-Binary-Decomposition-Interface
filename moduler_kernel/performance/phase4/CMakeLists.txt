cmake_minimum_required(VERSION 3.10)
project(BDI_Phase4 C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O3 -march=native")

# PGO options
option(ENABLE_PGO_INSTRUMENTATION "Enable PGO instrumentation" OFF)
option(ENABLE_PGO "Enable PGO optimization" OFF)
option(ENABLE_LTO "Enable Link-Time Optimization" OFF)

if(ENABLE_PGO_INSTRUMENTATION)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-generate")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-generate")
endif()

if(ENABLE_PGO)
    if(NOT PGO_PROFILE_FILE)
        message(FATAL_ERROR "PGO_PROFILE_FILE must be set when ENABLE_PGO is ON")
    endif()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-use=${PGO_PROFILE_FILE}")
endif()

if(ENABLE_LTO)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../phase1
    ${CMAKE_CURRENT_SOURCE_DIR}/../phase2
    ${CMAKE_CURRENT_SOURCE_DIR}/../phase3
)

# PGO subsystem
set(PGO_SOURCES
    pgo/pgo_profile.c
)

# Intrinsics subsystem
set(INTRINSICS_SOURCES
    intrinsics/cpu_features.c
)

# Profiler subsystem
set(PROFILER_SOURCES
    profiler/perf_collector.c
    profiler/ai_optimizer.c
)

# Integration
set(INTEGRATION_SOURCES
    integration/phase4_init.c
)

# Build Phase 4 library
add_library(bdi_phase4 STATIC
    ${PGO_SOURCES}
    ${INTRINSICS_SOURCES}
    ${PROFILER_SOURCES}
    ${INTEGRATION_SOURCES}
)

# Tests
enable_testing()

add_executable(test_cpu_features tests/test_cpu_features.c)
target_link_libraries(test_cpu_features bdi_phase4)
add_test(NAME test_cpu_features COMMAND test_cpu_features)

add_executable(test_perf_collector tests/test_perf_collector.c)
target_link_libraries(test_perf_collector bdi_phase4)
add_test(NAME test_perf_collector COMMAND test_perf_collector)

# Benchmarks
add_executable(bench_phase4 bench/bench_phase4.c)
target_link_libraries(bench_phase4 bdi_phase4 m)

# Install
install(TARGETS bdi_phase4 DESTINATION lib)
install(DIRECTORY pgo/ DESTINATION include/bdi/phase4/pgo FILES_MATCHING PATTERN "*.h")
install(DIRECTORY intrinsics/ DESTINATION include/bdi/phase4/intrinsics FILES_MATCHING PATTERN "*.h")
install(DIRECTORY profiler/ DESTINATION include/bdi/phase4/profiler FILES_MATCHING PATTERN "*.h")

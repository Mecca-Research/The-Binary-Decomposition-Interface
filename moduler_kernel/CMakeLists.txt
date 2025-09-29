
# ===================================================================
# BDI Modular Kernel CMake Build System
# Cross-platform build support with profile optimization
# ===================================================================

cmake_minimum_required(VERSION 3.16)
project(BDI_Modular_Kernel VERSION 0.1.0 LANGUAGES C CXX)

# ===================================================================
# Build Configuration
# ===================================================================

# Set default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# Build options
option(BDI_BUILD_TESTS "Build test suite" ON)
option(BDI_BUILD_BENCHMARKS "Build benchmarks" ON)
option(BDI_ENABLE_LTO "Enable Link Time Optimization" OFF)
option(BDI_ENABLE_NATIVE "Enable native CPU optimizations" ON)
option(BDI_ENABLE_ASAN "Enable AddressSanitizer" OFF)

# Profile selection
set(BDI_PROFILE "balanced" CACHE STRING "Optimization profile")
set_property(CACHE BDI_PROFILE PROPERTY STRINGS "latency" "throughput" "ai-train" "energy" "balanced")

# Architecture detection
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    set(BDI_ARCH "x86_64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
    set(BDI_ARCH "arm64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "riscv64")
    set(BDI_ARCH "riscv64")
else()
    set(BDI_ARCH "unknown")
    message(WARNING "Unknown architecture: ${CMAKE_SYSTEM_PROCESSOR}")
endif()

message(STATUS "BDI Modular Kernel Configuration:")
message(STATUS "  Profile: ${BDI_PROFILE}")
message(STATUS "  Architecture: ${BDI_ARCH}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")

# ===================================================================
# Compiler Configuration
# ===================================================================

# C/C++ standards
set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Common compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")

# Architecture-specific optimizations
if(BDI_ENABLE_NATIVE AND BDI_ARCH STREQUAL "x86_64")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=native -mtune=native")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -mtune=native")
    
    # Check for specific x86 features
    include(CheckCSourceCompiles)
    check_c_source_compiles("
        #include <immintrin.h>
        int main() { __m256i v = _mm256_setzero_si256(); return 0; }
    " HAVE_AVX2)
    
    if(HAVE_AVX2)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mavx2 -mfma")
        add_definitions(-DHAVE_AVX2)
    endif()
endif()

# Profile-specific optimizations
if(BDI_PROFILE STREQUAL "latency")
    add_definitions(-DBDI_PROFILE_LATENCY)
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -fno-omit-frame-pointer")
elseif(BDI_PROFILE STREQUAL "throughput")
    add_definitions(-DBDI_PROFILE_THROUGHPUT)
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3")
    set(BDI_ENABLE_LTO ON)
elseif(BDI_PROFILE STREQUAL "ai-train")
    add_definitions(-DBDI_PROFILE_AI_TRAIN)
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -ffast-math")
    set(BDI_ENABLE_LTO ON)
elseif(BDI_PROFILE STREQUAL "energy")
    add_definitions(-DBDI_PROFILE_ENERGY)
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Os")
else() # balanced
    add_definitions(-DBDI_PROFILE_BALANCED)
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2")
endif()

# Link Time Optimization
if(BDI_ENABLE_LTO)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
endif()

# AddressSanitizer
if(BDI_ENABLE_ASAN)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
endif()

# ===================================================================
# Dependencies
# ===================================================================

find_package(Threads REQUIRED)

# Math library
find_library(MATH_LIBRARY m)
if(MATH_LIBRARY)
    set(MATH_LIBRARIES ${MATH_LIBRARY})
endif()

# ===================================================================
# Source Files
# ===================================================================

# Core modular kernel sources
set(BDI_CORE_SOURCES
    uabi/uops.c
    capgraph/capability.c
    orchestrator/orchestrator.c
    attention_mm/attention_mm.c
    master_memory_manager/master_memory_manager.c
    master_memory_manager/toolchain.c
)

# Architecture-specific sources
if(BDI_ARCH STREQUAL "x86_64")
    list(APPEND BDI_CORE_SOURCES arch/x86/uops_x86.c)
elseif(BDI_ARCH STREQUAL "arm64")
    list(APPEND BDI_CORE_SOURCES arch/arm/uops_arm.c)
elseif(BDI_ARCH STREQUAL "riscv64")
    list(APPEND BDI_CORE_SOURCES arch/riscv/uops_riscv.c)
endif()

# Test sources
set(BDI_TEST_SOURCES
    tests/test_modular.c
)

# Integration sources
set(BDI_INTEGRATION_SOURCES
    integration/bdi_modular_main.c
)

# ===================================================================
# Libraries
# ===================================================================

# Core modular kernel library
add_library(bdi_modular STATIC ${BDI_CORE_SOURCES})
target_include_directories(bdi_modular PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(bdi_modular ${MATH_LIBRARIES} Threads::Threads)

# Set library properties
set_target_properties(bdi_modular PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "bdi_modular_${BDI_PROFILE}_${BDI_ARCH}"
)

# ===================================================================
# Executables
# ===================================================================

# Test executable
if(BDI_BUILD_TESTS)
    add_executable(bdi_modular_test ${BDI_TEST_SOURCES})
    target_link_libraries(bdi_modular_test bdi_modular)
    
    # Add test target
    enable_testing()
    add_test(NAME modular_tests COMMAND bdi_modular_test)
endif()

# Integration demo executable
add_executable(bdi_modular_demo ${BDI_INTEGRATION_SOURCES})
target_link_libraries(bdi_modular_demo bdi_modular)

# Try to link with existing BDI kernel if available
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../bdi_kernel")
    target_include_directories(bdi_modular_demo PRIVATE ../bdi_kernel)
    # Note: Would need to build existing BDI kernel as library
endif()

# ===================================================================
# Custom Targets
# ===================================================================

# Profile-specific build targets
add_custom_target(latency
    COMMAND ${CMAKE_COMMAND} -DBDI_PROFILE=latency ${CMAKE_SOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}
    COMMENT "Building with latency profile"
)

add_custom_target(throughput
    COMMAND ${CMAKE_COMMAND} -DBDI_PROFILE=throughput ${CMAKE_SOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}
    COMMENT "Building with throughput profile"
)

add_custom_target(ai-train
    COMMAND ${CMAKE_COMMAND} -DBDI_PROFILE=ai-train ${CMAKE_SOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}
    COMMENT "Building with AI training profile"
)

# Benchmark target
if(BDI_BUILD_BENCHMARKS AND BDI_BUILD_TESTS)
    add_custom_target(benchmark
        COMMAND bdi_modular_test --benchmark
        DEPENDS bdi_modular_test
        COMMENT "Running performance benchmarks"
    )
endif()

# Documentation target (placeholder)
add_custom_target(docs
    COMMAND echo "Documentation generation not implemented yet"
    COMMENT "Generating documentation"
)

# ===================================================================
# Installation
# ===================================================================

# Install library
install(TARGETS bdi_modular
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# Install headers
install(DIRECTORY uabi/ DESTINATION include/bdi/uabi
    FILES_MATCHING PATTERN "*.h")
install(DIRECTORY capgraph/ DESTINATION include/bdi/capgraph
    FILES_MATCHING PATTERN "*.h")
install(DIRECTORY orchestrator/ DESTINATION include/bdi/orchestrator
    FILES_MATCHING PATTERN "*.h")
install(DIRECTORY attention_mm/ DESTINATION include/bdi/attention_mm
    FILES_MATCHING PATTERN "*.h")
install(DIRECTORY master_memory_manager/ DESTINATION include/bdi/master_memory_manager
    FILES_MATCHING PATTERN "*.h")
install(DIRECTORY bricks/ DESTINATION include/bdi/bricks
    FILES_MATCHING PATTERN "*.h")

# Install executables
install(TARGETS bdi_modular_demo DESTINATION bin)
if(BDI_BUILD_TESTS)
    install(TARGETS bdi_modular_test DESTINATION bin)
endif()

# Install profiles
install(DIRECTORY profiles/ DESTINATION share/bdi/profiles
    FILES_MATCHING PATTERN "*.yaml")

# ===================================================================
# Package Configuration
# ===================================================================

# Generate package config files
include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/BDIModularConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/BDIModularConfig.cmake"
    INSTALL_DESTINATION lib/cmake/BDIModular
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/BDIModularConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/BDIModularConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/BDIModularConfigVersion.cmake"
    DESTINATION lib/cmake/BDIModular
)

# ===================================================================
# Subdirectories
# ===================================================================

# Add Master Memory Manager subdirectory
add_subdirectory(master_memory_manager)

# ===================================================================
# Summary
# ===================================================================

message(STATUS "BDI Modular Kernel build configured:")
message(STATUS "  Targets: bdi_modular (library)")
if(BDI_BUILD_TESTS)
    message(STATUS "           bdi_modular_test (tests)")
endif()
message(STATUS "           bdi_modular_demo (integration demo)")
message(STATUS "           master_memory_manager (Phase 2 library)")
if(BDI_BUILD_TESTS)
    message(STATUS "           test_master_memory_manager (Phase 2 tests)")
endif()
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  Compiler flags: ${CMAKE_C_FLAGS}")
if(BDI_ENABLE_LTO)
    message(STATUS "  LTO: Enabled")
endif()
if(BDI_ENABLE_ASAN)
    message(STATUS "  AddressSanitizer: Enabled")
endif()

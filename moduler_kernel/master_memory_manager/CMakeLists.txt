
# Master Memory Manager - Phase 4 LEGENDARY BDI BUILD
# Complete CMake configuration for all phases

cmake_minimum_required(VERSION 3.16)
project(MasterMemoryManager VERSION 4.0.0 LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Build configuration
set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type")
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0 -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -DNDEBUG -march=native")

# Feature flags
option(MMM_ENABLE_AI "Enable AI capabilities" ON)
option(MMM_ENABLE_SECURITY "Enable security features" ON)
option(MMM_ENABLE_TELEMETRY "Enable telemetry" ON)
option(MMM_ENABLE_TESTING "Enable testing" OFF)
option(MMM_ENABLE_PROFILING "Enable profiling" OFF)
option(MMM_PRODUCTION_BUILD "Production build optimizations" OFF)

# Find required packages
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)

# Optional packages
if(MMM_ENABLE_AI)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(BLAS blas)
    pkg_check_modules(LAPACK lapack)
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/hal_framework
    ${CMAKE_CURRENT_SOURCE_DIR}/x86_core
    ${CMAKE_CURRENT_SOURCE_DIR}/system_integration
    ${CMAKE_CURRENT_SOURCE_DIR}/toolchain
    ${CMAKE_CURRENT_SOURCE_DIR}/phase3_ai_assembly_engineers
    ${CMAKE_CURRENT_SOURCE_DIR}/phase4_master_control
    ${CMAKE_CURRENT_SOURCE_DIR}/production_features
    ${CMAKE_CURRENT_SOURCE_DIR}/enterprise_features
    ${CMAKE_CURRENT_SOURCE_DIR}/ai_capabilities
)

# Source files - Phase 1: HAL Framework
set(HAL_SOURCES
    hal_framework/bsp/mmm_bsp.c
    hal_framework/hardware_access/mmm_hardware_access.c
    hal_framework/peripheral_drivers/mmm_peripheral_drivers.c
)

# Source files - Phase 1: x86 Core
set(X86_SOURCES
    x86_core/atomic_ops/x86_atomic_ops.c
    x86_core/cache_hints/x86_cache_hints.c
    x86_core/calling_abi/x86_calling_abi.c
    x86_core/dma_pcie/x86_dma_pcie.c
    x86_core/interrupts_idt/x86_interrupts_idt.c
    x86_core/paging_mmu/x86_paging_mmu.c
    x86_core/registers/x86_registers.c
    x86_core/simd_avx/x86_simd_avx.c
    x86_core/task_switching/x86_task_switching.c
    x86_core/tlb_mgmt/x86_tlb_mgmt.c
)

# Source files - Phase 2: System Integration
set(SYSTEM_INTEGRATION_SOURCES
    system_integration/interrupt_mgmt/mmm_interrupt_mgmt.c
    system_integration/memory_protection/mmm_memory_protection.c
    system_integration/performance/mmm_performance.c
)

# Source files - Phase 2: Toolchain
set(TOOLCHAIN_SOURCES
    toolchain/bdi_parser/bdi_parser.c
    toolchain/multi_rail_synthesis/multi_rail_synthesis.c
)

# Source files - Phase 3: AI Assembly Engineers
set(PHASE3_SOURCES
    phase3_ai_assembly_engineers/integration/phase3_main.c
    phase3_ai_assembly_engineers/runtime/capsule_loader/capsule_loader.c
    phase3_ai_assembly_engineers/runtime/hotswap_lanes/hotswap_manager.c
)

# Source files - Phase 4: Master Control
set(PHASE4_MASTER_CONTROL_SOURCES
    phase4_master_control/orchestrator/mmm_orchestrator.c
    phase4_master_control/bdi_integration/mmm_bdi_integration.c
)

# Source files - Core MMM
set(CORE_SOURCES
    master_memory_manager.c
)

# Main Phase 4 source
set(PHASE4_MAIN_SOURCE
    phase4_main.c
)

# All sources
set(ALL_SOURCES
    ${CORE_SOURCES}
    ${HAL_SOURCES}
    ${X86_SOURCES}
    ${SYSTEM_INTEGRATION_SOURCES}
    ${TOOLCHAIN_SOURCES}
    ${PHASE3_SOURCES}
    ${PHASE4_MASTER_CONTROL_SOURCES}
)

# Create main executable
add_executable(mmm_phase4 ${PHASE4_MAIN_SOURCE} ${ALL_SOURCES})

# Link libraries
target_link_libraries(mmm_phase4 
    Threads::Threads
    m  # Math library
    rt # Real-time library
)

# Conditional linking
if(MMM_ENABLE_AI AND BLAS_FOUND AND LAPACK_FOUND)
    target_link_libraries(mmm_phase4 ${BLAS_LIBRARIES} ${LAPACK_LIBRARIES})
    target_compile_definitions(mmm_phase4 PRIVATE MMM_AI_ENABLED)
endif()

# Feature definitions
if(MMM_ENABLE_SECURITY)
    target_compile_definitions(mmm_phase4 PRIVATE MMM_SECURITY_ENABLED)
endif()

if(MMM_ENABLE_TELEMETRY)
    target_compile_definitions(mmm_phase4 PRIVATE MMM_TELEMETRY_ENABLED)
endif()

if(MMM_PRODUCTION_BUILD)
    target_compile_definitions(mmm_phase4 PRIVATE MMM_PRODUCTION_BUILD)
    set_target_properties(mmm_phase4 PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION TRUE
    )
endif()

# Testing
if(MMM_ENABLE_TESTING)
    enable_testing()
    
    # Phase 4 tests
    add_executable(test_master_control 
        tests/test_master_memory_manager.c
        ${ALL_SOURCES}
    )
    target_link_libraries(test_master_control Threads::Threads m rt)
    add_test(NAME MasterControlTest COMMAND test_master_control)
    
    # Add more test executables as needed
    file(GLOB TEST_SOURCES "tests/phase4/*.c")
    foreach(TEST_SOURCE ${TEST_SOURCES})
        get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
        add_executable(${TEST_NAME} ${TEST_SOURCE} ${ALL_SOURCES})
        target_link_libraries(${TEST_NAME} Threads::Threads m rt)
        add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    endforeach()
endif()

# Profiling
if(MMM_ENABLE_PROFILING)
    target_compile_options(mmm_phase4 PRIVATE -pg)
    target_link_options(mmm_phase4 PRIVATE -pg)
endif()

# Installation
install(TARGETS mmm_phase4 
    RUNTIME DESTINATION bin
)

# Install headers for development
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/mmm
    FILES_MATCHING PATTERN "*.h"
)

# Install documentation
install(FILES 
    README.md
    DESTINATION share/doc/mmm
)

# CPack configuration for packaging
set(CPACK_PACKAGE_NAME "MasterMemoryManager")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Master Memory Manager - LEGENDARY BDI BUILD")
set(CPACK_PACKAGE_VENDOR "Mecca Research")
set(CPACK_PACKAGE_CONTACT "research@mecca.ai")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

# Package generators
set(CPACK_GENERATOR "TGZ;DEB;RPM")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libpthread-stubs0-dev")
set(CPACK_RPM_PACKAGE_REQUIRES "glibc, glibc-devel")

include(CPack)

# Custom targets
add_custom_target(format
    COMMAND find ${CMAKE_CURRENT_SOURCE_DIR} -name "*.c" -o -name "*.h" | xargs clang-format -i
    COMMENT "Formatting source code"
)

add_custom_target(docs
    COMMAND doxygen ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating documentation"
)

# Print configuration summary
message(STATUS "=== Master Memory Manager Phase 4 Configuration ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "AI capabilities: ${MMM_ENABLE_AI}")
message(STATUS "Security features: ${MMM_ENABLE_SECURITY}")
message(STATUS "Telemetry: ${MMM_ENABLE_TELEMETRY}")
message(STATUS "Testing: ${MMM_ENABLE_TESTING}")
message(STATUS "Profiling: ${MMM_ENABLE_PROFILING}")
message(STATUS "Production build: ${MMM_PRODUCTION_BUILD}")
message(STATUS "=== LEGENDARY BDI BUILD Ready ===")

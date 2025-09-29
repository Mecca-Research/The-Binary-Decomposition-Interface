
# Master Memory Manager - Phase 1 CMake Configuration
# AI Assembly Engineers for BDI

cmake_minimum_required(VERSION 3.20)
project(MasterMemoryManager VERSION 1.0.0 LANGUAGES C)

# Set C23 standard
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Compiler flags for optimization and debugging
set(CMAKE_C_FLAGS_DEBUG "-g -O0 -DDEBUG -Wall -Wextra -Wpedantic")
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG -Wall -Wextra")

# Default to Release build
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Add Phase 3: AI Assembly Engineers for BDI
add_subdirectory(phase3_ai_assembly_engineers)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/x86_core/registers
    ${CMAKE_CURRENT_SOURCE_DIR}/x86_core/calling_abi
    ${CMAKE_CURRENT_SOURCE_DIR}/x86_core/paging_mmu
    ${CMAKE_CURRENT_SOURCE_DIR}/x86_core/tlb_mgmt
    ${CMAKE_CURRENT_SOURCE_DIR}/x86_core/cache_hints
    ${CMAKE_CURRENT_SOURCE_DIR}/hal_framework/bsp
    ${CMAKE_CURRENT_SOURCE_DIR}/hal_framework/peripheral_drivers
    ${CMAKE_CURRENT_SOURCE_DIR}/hal_framework/hardware_access
    ${CMAKE_CURRENT_SOURCE_DIR}/system_integration/interrupt_mgmt
    ${CMAKE_CURRENT_SOURCE_DIR}/system_integration/memory_protection
    ${CMAKE_CURRENT_SOURCE_DIR}/system_integration/performance
)

# Source files
set(MMM_SOURCES
    # Core Master Memory Manager
    master_memory_manager.c
    
    # x86 Core Competencies
    x86_core/registers/x86_registers.c
    x86_core/calling_abi/x86_calling_abi.c
    x86_core/paging_mmu/x86_paging_mmu.c
    x86_core/tlb_mgmt/x86_tlb_mgmt.c
    x86_core/cache_hints/x86_cache_hints.c
    
    # HAL Framework
    hal_framework/bsp/mmm_bsp.c
    hal_framework/peripheral_drivers/mmm_peripheral_drivers.c
    hal_framework/hardware_access/mmm_hardware_access.c
    
    # System Integration
    system_integration/interrupt_mgmt/mmm_interrupt_mgmt.c
    system_integration/memory_protection/mmm_memory_protection.c
    system_integration/performance/mmm_performance.c
)

# Header files
set(MMM_HEADERS
    # Core Master Memory Manager
    master_memory_manager.h
    
    # x86 Core Competencies
    x86_core/registers/x86_registers.h
    x86_core/calling_abi/x86_calling_abi.h
    x86_core/paging_mmu/x86_paging_mmu.h
    x86_core/tlb_mgmt/x86_tlb_mgmt.h
    x86_core/cache_hints/x86_cache_hints.h
    
    # HAL Framework
    hal_framework/bsp/mmm_bsp.h
    hal_framework/peripheral_drivers/mmm_peripheral_drivers.h
    hal_framework/hardware_access/mmm_hardware_access.h
    
    # System Integration
    system_integration/interrupt_mgmt/mmm_interrupt_mgmt.h
    system_integration/memory_protection/mmm_memory_protection.h
    system_integration/performance/mmm_performance.h
)

# Create Master Memory Manager library
add_library(master_memory_manager STATIC ${MMM_SOURCES} ${MMM_HEADERS})

# Set library properties
set_target_properties(master_memory_manager PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER "${MMM_HEADERS}"
)

# Compiler-specific optimizations
if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(master_memory_manager PRIVATE
        -march=native
        -mtune=native
        -ffast-math
        -funroll-loops
        -finline-functions
        -fomit-frame-pointer
    )
elseif(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(master_memory_manager PRIVATE
        -march=native
        -mtune=native
        -ffast-math
        -funroll-loops
        -finline-functions
        -fomit-frame-pointer
    )
endif()

# Link libraries
target_link_libraries(master_memory_manager PRIVATE m)

# Test executable
add_executable(test_master_memory_manager
    tests/test_master_memory_manager.c
)

target_link_libraries(test_master_memory_manager PRIVATE master_memory_manager)

# Enable testing
enable_testing()
add_test(NAME MasterMemoryManagerTests COMMAND test_master_memory_manager)

# Installation
install(TARGETS master_memory_manager
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION include/master_memory_manager
)

install(FILES README.md DESTINATION share/doc/master_memory_manager)

# Custom targets
add_custom_target(format
    COMMAND find ${CMAKE_CURRENT_SOURCE_DIR} -name "*.c" -o -name "*.h" | xargs clang-format -i
    COMMENT "Formatting source code"
)

add_custom_target(analyze
    COMMAND cppcheck --enable=all --std=c23 ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running static analysis"
)

# Print configuration summary
message(STATUS "Master Memory Manager Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C Standard: C${CMAKE_C_STANDARD}")
message(STATUS "  Compiler: ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")

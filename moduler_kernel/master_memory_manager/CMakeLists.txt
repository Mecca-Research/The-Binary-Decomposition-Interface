
# ===================================================================
# BDI Master Memory Manager - Phase 2 CMake Build System
# Advanced x86 systems, toolchain, multi-rail synthesis
# ===================================================================

cmake_minimum_required(VERSION 3.16)

# ===================================================================
# Master Memory Manager Library
# ===================================================================

# Core sources
set(MMM_CORE_SOURCES
    master_memory_manager.c
    toolchain.c
)

# Headers
set(MMM_HEADERS
    master_memory_manager.h
    x86_advanced.h
    toolchain.h
)

# Create master memory manager library
add_library(master_memory_manager STATIC ${MMM_CORE_SOURCES})

# Include directories
target_include_directories(master_memory_manager PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Link with parent library
target_link_libraries(master_memory_manager bdi_modular)

# Compiler-specific flags for x86 features
if(BDI_ARCH STREQUAL "x86_64")
    # Enable x86-specific features
    target_compile_definitions(master_memory_manager PRIVATE
        MMM_X86_64_SUPPORT=1
        MMM_SIMD_SUPPORT=1
    )
    
    # Check for CPUID support
    include(CheckCSourceCompiles)
    check_c_source_compiles("
        #include <cpuid.h>
        int main() { 
            unsigned int eax, ebx, ecx, edx;
            return __get_cpuid(1, &eax, &ebx, &ecx, &edx) ? 0 : 1;
        }
    " HAVE_CPUID)
    
    if(HAVE_CPUID)
        target_compile_definitions(master_memory_manager PRIVATE HAVE_CPUID=1)
    endif()
    
    # Check for SIMD intrinsics
    check_c_source_compiles("
        #include <immintrin.h>
        int main() { 
            __m256i v = _mm256_setzero_si256(); 
            return 0; 
        }
    " HAVE_AVX2_INTRINSICS)
    
    if(HAVE_AVX2_INTRINSICS)
        target_compile_definitions(master_memory_manager PRIVATE HAVE_AVX2_INTRINSICS=1)
        target_compile_options(master_memory_manager PRIVATE -mavx2)
    endif()
    
    # Check for AVX-512 support
    check_c_source_compiles("
        #include <immintrin.h>
        int main() { 
            __m512i v = _mm512_setzero_si512(); 
            return 0; 
        }
    " HAVE_AVX512_INTRINSICS)
    
    if(HAVE_AVX512_INTRINSICS)
        target_compile_definitions(master_memory_manager PRIVATE HAVE_AVX512_INTRINSICS=1)
        target_compile_options(master_memory_manager PRIVATE -mavx512f)
    endif()
endif()

# Profile-specific optimizations for MMM
if(BDI_PROFILE STREQUAL "latency")
    target_compile_definitions(master_memory_manager PRIVATE
        MMM_OPTIMIZE_LATENCY=1
        MMM_INLINE_CRITICAL_PATHS=1
    )
elseif(BDI_PROFILE STREQUAL "throughput")
    target_compile_definitions(master_memory_manager PRIVATE
        MMM_OPTIMIZE_THROUGHPUT=1
        MMM_ENABLE_VECTORIZATION=1
    )
elseif(BDI_PROFILE STREQUAL "ai-train")
    target_compile_definitions(master_memory_manager PRIVATE
        MMM_OPTIMIZE_AI_TRAINING=1
        MMM_ENABLE_MIXED_PRECISION=1
    )
endif()

# Set library properties
set_target_properties(master_memory_manager PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "master_memory_manager_${BDI_PROFILE}_${BDI_ARCH}"
    C_STANDARD 23
    C_STANDARD_REQUIRED ON
)

# ===================================================================
# Test Suite
# ===================================================================

if(BDI_BUILD_TESTS)
    # Test sources
    set(MMM_TEST_SOURCES
        tests/test_master_memory_manager.c
    )
    
    # Create test executable
    add_executable(test_master_memory_manager ${MMM_TEST_SOURCES})
    
    # Link with MMM library
    target_link_libraries(test_master_memory_manager 
        master_memory_manager
        bdi_modular
        ${MATH_LIBRARIES}
        Threads::Threads
    )
    
    # Test-specific compile definitions
    target_compile_definitions(test_master_memory_manager PRIVATE
        MMM_TESTING=1
        MMM_DEBUG_MODE=1
    )
    
    # Add to test suite
    add_test(NAME master_memory_manager_tests 
             COMMAND test_master_memory_manager)
    
    # Set test properties
    set_tests_properties(master_memory_manager_tests PROPERTIES
        TIMEOUT 300
        LABELS "master_memory_manager;phase2"
    )
endif()

# ===================================================================
# Benchmarks
# ===================================================================

if(BDI_BUILD_BENCHMARKS)
    # Check if benchmark source exists
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/benchmark_master_memory_manager.c")
        # Benchmark sources
        set(MMM_BENCHMARK_SOURCES
            benchmarks/benchmark_master_memory_manager.c
        )
        
        # Create benchmark executable
        add_executable(benchmark_master_memory_manager ${MMM_BENCHMARK_SOURCES})
        
        # Link with MMM library
        target_link_libraries(benchmark_master_memory_manager 
            master_memory_manager
            bdi_modular
            ${MATH_LIBRARIES}
            Threads::Threads
        )
        
        # Benchmark-specific optimizations
        target_compile_options(benchmark_master_memory_manager PRIVATE
            -O3 -DNDEBUG -fno-omit-frame-pointer
        )
        
        target_compile_definitions(benchmark_master_memory_manager PRIVATE
            MMM_BENCHMARKING=1
            MMM_PERFORMANCE_COUNTERS=1
        )
        
        set(MMM_BENCHMARKS_AVAILABLE TRUE)
    else()
        message(STATUS "Master Memory Manager benchmarks not available (missing source file)")
        set(MMM_BENCHMARKS_AVAILABLE FALSE)
    endif()
endif()

# ===================================================================
# Documentation
# ===================================================================

# Find Doxygen
find_package(Doxygen QUIET)

if(DOXYGEN_FOUND)
    # Configure Doxygen
    set(DOXYGEN_PROJECT_NAME "BDI Master Memory Manager")
    set(DOXYGEN_PROJECT_BRIEF "Phase 2: Advanced x86 Systems & Toolchain")
    set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/docs")
    set(DOXYGEN_INPUT "${CMAKE_CURRENT_SOURCE_DIR}")
    set(DOXYGEN_RECURSIVE YES)
    set(DOXYGEN_EXTRACT_ALL YES)
    set(DOXYGEN_GENERATE_HTML YES)
    set(DOXYGEN_GENERATE_LATEX NO)
    set(DOXYGEN_USE_MDFILE_AS_MAINPAGE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
    
    # Create documentation target
    doxygen_add_docs(mmm_docs
        ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating Master Memory Manager documentation"
    )
    
    # Add to main docs target
    add_dependencies(docs mmm_docs)
endif()

# ===================================================================
# Installation
# ===================================================================

# Install library
install(TARGETS master_memory_manager
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# Install headers
install(FILES ${MMM_HEADERS}
    DESTINATION include/bdi/master_memory_manager
)

# Install test executable
if(BDI_BUILD_TESTS)
    install(TARGETS test_master_memory_manager
        DESTINATION bin
    )
endif()

# Install benchmark executable
if(BDI_BUILD_BENCHMARKS AND MMM_BENCHMARKS_AVAILABLE)
    install(TARGETS benchmark_master_memory_manager
        DESTINATION bin
    )
endif()

# ===================================================================
# Custom Targets
# ===================================================================

# MMM-specific build targets
add_custom_target(mmm-latency
    COMMAND ${CMAKE_COMMAND} -DBDI_PROFILE=latency ${CMAKE_SOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target master_memory_manager
    COMMENT "Building Master Memory Manager with latency optimization"
)

add_custom_target(mmm-throughput
    COMMAND ${CMAKE_COMMAND} -DBDI_PROFILE=throughput ${CMAKE_SOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target master_memory_manager
    COMMENT "Building Master Memory Manager with throughput optimization"
)

add_custom_target(mmm-ai-train
    COMMAND ${CMAKE_COMMAND} -DBDI_PROFILE=ai-train ${CMAKE_SOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target master_memory_manager
    COMMENT "Building Master Memory Manager with AI training optimization"
)

# Test targets
if(BDI_BUILD_TESTS)
    add_custom_target(mmm-test
        COMMAND test_master_memory_manager
        DEPENDS test_master_memory_manager
        COMMENT "Running Master Memory Manager tests"
    )
    
    add_custom_target(mmm-test-verbose
        COMMAND test_master_memory_manager --verbose
        DEPENDS test_master_memory_manager
        COMMENT "Running Master Memory Manager tests (verbose)"
    )
endif()

# Benchmark targets
if(BDI_BUILD_BENCHMARKS AND MMM_BENCHMARKS_AVAILABLE)
    add_custom_target(mmm-benchmark
        COMMAND benchmark_master_memory_manager
        DEPENDS benchmark_master_memory_manager
        COMMENT "Running Master Memory Manager benchmarks"
    )
    
    add_custom_target(mmm-benchmark-report
        COMMAND benchmark_master_memory_manager --report
        DEPENDS benchmark_master_memory_manager
        COMMENT "Running Master Memory Manager benchmarks with detailed report"
    )
endif()

# Analysis targets
add_custom_target(mmm-analyze
    COMMAND echo "Master Memory Manager Analysis:"
    COMMAND echo "  Source files: ${MMM_CORE_SOURCES}"
    COMMAND echo "  Header files: ${MMM_HEADERS}"
    COMMAND wc -l ${MMM_CORE_SOURCES} ${MMM_HEADERS} || true
    COMMENT "Analyzing Master Memory Manager codebase"
)

# ===================================================================
# Summary
# ===================================================================

message(STATUS "Master Memory Manager Phase 2 configured:")
message(STATUS "  Profile: ${BDI_PROFILE}")
message(STATUS "  Architecture: ${BDI_ARCH}")
if(HAVE_CPUID)
    message(STATUS "  CPUID support: Yes")
endif()
if(HAVE_AVX2_INTRINSICS)
    message(STATUS "  AVX2 intrinsics: Yes")
endif()
if(HAVE_AVX512_INTRINSICS)
    message(STATUS "  AVX-512 intrinsics: Yes")
endif()
if(BDI_BUILD_TESTS)
    message(STATUS "  Tests: Enabled")
endif()
if(BDI_BUILD_BENCHMARKS)
    if(MMM_BENCHMARKS_AVAILABLE)
        message(STATUS "  Benchmarks: Enabled")
    else()
        message(STATUS "  Benchmarks: Disabled (source not available)")
    endif()
endif()
if(DOXYGEN_FOUND)
    message(STATUS "  Documentation: Enabled")
endif()

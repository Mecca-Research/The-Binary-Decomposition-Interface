
cmake_minimum_required(VERSION 3.15)
project(BDI_Phase2 C)

# C23 standard with fallback
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED OFF)
set(CMAKE_C_EXTENSIONS ON)

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -march=native")

# Feature detection
include(CheckCSourceCompiles)

# Check for AVX2 support
check_c_source_compiles("
    #include <immintrin.h>
    int main() {
        __m256i a = _mm256_setzero_si256();
        return 0;
    }
" HAVE_AVX2)

if(HAVE_AVX2)
    add_definitions(-DHAVE_AVX2)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mavx2")
endif()

# Check for 128-bit integer support
check_c_source_compiles("
    int main() {
        __uint128_t x = 0;
        return 0;
    }
" HAVE_INT128)

if(HAVE_INT128)
    add_definitions(-DHAVE_INT128)
endif()

# Sanitizer support
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(ENABLE_MSAN "Enable MemorySanitizer" OFF)

if(ENABLE_ASAN)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
endif()

if(ENABLE_UBSAN)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=undefined")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=undefined")
endif()

if(ENABLE_MSAN)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=memory -fno-omit-frame-pointer")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=memory")
endif()

# BCI Library
add_library(bci STATIC
    bci/bci_arithmetic.c
    bci/bci_bitops.c
    bci/bci_conversion.c
    bci/bci_simd.c
    bci/chimera_bci.c
)

target_include_directories(bci PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# BTL Library
add_library(btl STATIC
    btl/btl_isa.c
    btl/btl_regalloc.c
    btl/btl_scheduler.c
    btl/btl_peephole.c
    btl/chimera_btl.c
)

target_include_directories(btl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Enable testing
enable_testing()

# BCI Tests
add_executable(test_bci_arithmetic tests/test_bci_arithmetic.c)
target_link_libraries(test_bci_arithmetic bci)
add_test(NAME BCI_Arithmetic COMMAND test_bci_arithmetic)

add_executable(test_bci_bitops tests/test_bci_bitops.c)
target_link_libraries(test_bci_bitops bci)
add_test(NAME BCI_BitOps COMMAND test_bci_bitops)

add_executable(test_bci_conversion tests/test_bci_conversion.c)
target_link_libraries(test_bci_conversion bci)
add_test(NAME BCI_Conversion COMMAND test_bci_conversion)

add_executable(test_bci_simd tests/test_bci_simd.c)
target_link_libraries(test_bci_simd bci)
add_test(NAME BCI_SIMD COMMAND test_bci_simd)

# BTL Tests
add_executable(test_btl_isa tests/test_btl_isa.c)
target_link_libraries(test_btl_isa btl)
add_test(NAME BTL_ISA COMMAND test_btl_isa)

add_executable(test_btl_regalloc tests/test_btl_regalloc.c)
target_link_libraries(test_btl_regalloc btl)
add_test(NAME BTL_RegAlloc COMMAND test_btl_regalloc)

add_executable(test_btl_scheduler tests/test_btl_scheduler.c)
target_link_libraries(test_btl_scheduler btl)
add_test(NAME BTL_Scheduler COMMAND test_btl_scheduler)

add_executable(test_btl_peephole tests/test_btl_peephole.c)
target_link_libraries(test_btl_peephole btl)
add_test(NAME BTL_Peephole COMMAND test_btl_peephole)

# Benchmarks
add_executable(benchmark_bci tests/benchmark_bci.c)
target_link_libraries(benchmark_bci bci)

add_executable(benchmark_btl tests/benchmark_btl.c)
target_link_libraries(benchmark_btl btl)

# Installation
install(TARGETS bci btl
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(DIRECTORY bci/ btl/
    DESTINATION include/bdi
    FILES_MATCHING PATTERN "*.h"
)

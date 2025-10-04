cmake_minimum_required(VERSION 3.22)
project(BDI LANGUAGES CXX)

option(BDI_BUILD_TESTS "Build unit tests" ON)
option(BDI_SANITIZE "Enable sanitizers (ASan/UBSan)" OFF)
option(BDI_ENABLE_MSAN "Enable memory sanitizer" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Feature detection for C23 support
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-std=c++23" COMPILER_SUPPORTS_CXX23)
if(COMPILER_SUPPORTS_CXX23)
    set(CMAKE_CXX_STANDARD 23)
    add_compile_definitions(BDI_HAS_CXX23)
endif()

# Collect all source files
file(GLOB_RECURSE BDI_CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/core/**/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/compiler/**/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/optimizer/**/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/frontend/**/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/**/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/meta/**/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/hal/**/*.cpp
)

# Create main library
add_library(bdi STATIC ${BDI_CORE_SOURCES})

target_include_directories(bdi PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/compiler
    ${CMAKE_CURRENT_SOURCE_DIR}/optimizer
    ${CMAKE_CURRENT_SOURCE_DIR}/frontend
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime
    ${CMAKE_CURRENT_SOURCE_DIR}/meta
)

# Compiler warnings
target_compile_options(bdi PRIVATE
    -Wall -Wextra -Wpedantic
    $<$<CONFIG:Debug>:-O0 -g>
    $<$<CONFIG:Release>:-O3 -DNDEBUG>
    $<$<CONFIG:RelWithDebInfo>:-O2 -g>
)

# Sanitizers
if(BDI_SANITIZE)
    target_compile_options(bdi PUBLIC 
        -fsanitize=address,undefined 
        -fno-omit-frame-pointer
        -fno-optimize-sibling-calls
    )
    target_link_options(bdi PUBLIC 
        -fsanitize=address,undefined
    )
endif()

if(BDI_ENABLE_MSAN)
    target_compile_options(bdi PUBLIC 
        -fsanitize=memory
        -fno-omit-frame-pointer
    )
    target_link_options(bdi PUBLIC 
        -fsanitize=memory
    )
endif()

# Tests
if(BDI_BUILD_TESTS)
    enable_testing()
    
    # Find or fetch Google Test
    find_package(GTest QUIET)
    if(NOT GTest_FOUND)
        include(FetchContent)
        FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG release-1.12.1
        )
        FetchContent_MakeAvailable(googletest)
    endif()
    
    # Phase 4 tests
    file(GLOB PHASE4_TEST_SOURCES test/phase4/*.cpp)
    foreach(test_source ${PHASE4_TEST_SOURCES})
        get_filename_component(test_name ${test_source} NAME_WE)
        add_executable(${test_name} ${test_source})
        target_link_libraries(${test_name} PRIVATE bdi GTest::gtest GTest::gtest_main)
        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach()
    
    # Existing tests
    file(GLOB EXISTING_TEST_SOURCES test/*.cpp)
    foreach(test_source ${EXISTING_TEST_SOURCES})
        get_filename_component(test_name ${test_source} NAME_WE)
        if(NOT TARGET ${test_name})
            add_executable(${test_name} ${test_source})
            target_link_libraries(${test_name} PRIVATE bdi GTest::gtest GTest::gtest_main)
            add_test(NAME ${test_name} COMMAND ${test_name})
        endif()
    endforeach()
endif()

# Example executables
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/example/hello_graph/main.cpp)
    add_executable(bdi_hello example/hello_graph/main.cpp)
    target_link_libraries(bdi_hello PRIVATE bdi)
endif()

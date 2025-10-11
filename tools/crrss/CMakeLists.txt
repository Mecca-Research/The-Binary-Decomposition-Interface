# ===================================================================
# CRRSS - Code Review, Reliability, and Static Safety System
# CMake Build Configuration
# ===================================================================

cmake_minimum_required(VERSION 3.16)
project(CRRSS VERSION 1.0.0 LANGUAGES C)

# ===================================================================
# Build Configuration
# ===================================================================

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Build options
option(CRRSS_BUILD_TESTS "Build CRRSS test suite" ON)
option(CRRSS_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(CRRSS_BUILD_DOCS "Build documentation" OFF)

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3")

# AddressSanitizer
if(CRRSS_ENABLE_ASAN)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
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

# Common sources
set(CRRSS_COMMON_SOURCES
    common/crrss_types.c
)

# BPME sources
set(CRRSS_BPME_SOURCES
    bpme/bpme.c
)

# SCIV sources
set(CRRSS_SCIV_SOURCES
    sciv/sciv.c
)

# Memory Integration sources
set(CRRSS_MEMORY_SOURCES
    memory_layer/memory_integration.c
)

# CLI sources
set(CRRSS_CLI_SOURCES
    cli/crrss_cli.c
)

# All library sources
set(CRRSS_LIB_SOURCES
    ${CRRSS_COMMON_SOURCES}
    ${CRRSS_BPME_SOURCES}
    ${CRRSS_SCIV_SOURCES}
    ${CRRSS_MEMORY_SOURCES}
    ${CRRSS_CLI_SOURCES}
)

# ===================================================================
# Libraries
# ===================================================================

# CRRSS static library
add_library(crrss STATIC ${CRRSS_LIB_SOURCES})
target_include_directories(crrss PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(crrss ${MATH_LIBRARIES} Threads::Threads)

set_target_properties(crrss PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "crrss"
)

# ===================================================================
# Executables
# ===================================================================

# Main CRRSS command-line tool
add_executable(crrss_tool cli/crrss_main.c)
target_link_libraries(crrss_tool crrss)
set_target_properties(crrss_tool PROPERTIES OUTPUT_NAME "crrss")

# ===================================================================
# Tests
# ===================================================================

if(CRRSS_BUILD_TESTS)
    enable_testing()
    
    # BPME tests
    add_executable(test_bpme tests/test_bpme.c)
    target_link_libraries(test_bpme crrss)
    add_test(NAME bpme_tests COMMAND test_bpme)
    
    # SCIV tests
    add_executable(test_sciv tests/test_sciv.c)
    target_link_libraries(test_sciv crrss)
    add_test(NAME sciv_tests COMMAND test_sciv)
    
    # Memory Integration tests
    add_executable(test_memory tests/test_memory.c)
    target_link_libraries(test_memory crrss)
    add_test(NAME memory_tests COMMAND test_memory)
    
    # Add custom test target
    add_custom_target(check
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        DEPENDS test_bpme test_sciv test_memory
        COMMENT "Running CRRSS test suite"
    )
endif()

# ===================================================================
# Installation
# ===================================================================

# Install library
install(TARGETS crrss
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# Install headers
install(DIRECTORY common/ DESTINATION include/crrss/common
    FILES_MATCHING PATTERN "*.h")
install(DIRECTORY bpme/ DESTINATION include/crrss/bpme
    FILES_MATCHING PATTERN "*.h")
install(DIRECTORY sciv/ DESTINATION include/crrss/sciv
    FILES_MATCHING PATTERN "*.h")
install(DIRECTORY memory_layer/ DESTINATION include/crrss/memory_layer
    FILES_MATCHING PATTERN "*.h")
install(DIRECTORY cli/ DESTINATION include/crrss/cli
    FILES_MATCHING PATTERN "*.h")

# Install executable
install(TARGETS crrss_tool DESTINATION bin)

# Install documentation
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/docs")
    install(DIRECTORY docs/ DESTINATION share/doc/crrss)
endif()

# ===================================================================
# Package Configuration
# ===================================================================

include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CRRSSConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/CRRSSConfig.cmake"
    INSTALL_DESTINATION lib/cmake/CRRSS
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/CRRSSConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# ===================================================================
# Documentation
# ===================================================================

if(CRRSS_BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/docs")
        set(DOXYGEN_PROJECT_NAME "CRRSS")
        set(DOXYGEN_PROJECT_BRIEF "Code Review, Reliability, and Static Safety System")
        
        doxygen_add_docs(docs
            ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Generating CRRSS documentation"
        )
    endif()
endif()

# ===================================================================
# Summary
# ===================================================================

message(STATUS "CRRSS Build Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C Standard: ${CMAKE_C_STANDARD}")
message(STATUS "  Compiler: ${CMAKE_C_COMPILER_ID}")
if(CRRSS_BUILD_TESTS)
    message(STATUS "  Tests: Enabled")
endif()
if(CRRSS_ENABLE_ASAN)
    message(STATUS "  AddressSanitizer: Enabled")
endif()
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")

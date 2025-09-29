
# CMakeLists.txt for Phase 3: AI Assembly Engineers for BDI
cmake_minimum_required(VERSION 3.20)

project(Phase3_AI_Assembly_Engineers
    VERSION 1.0.0
    DESCRIPTION "AI Assembly Engineers for BDI - Complete Training and Runtime System"
    LANGUAGES C CXX
)

# Set C and C++ standards
set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic -O2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -O2")

# Debug flags
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -DDEBUG -fsanitize=address")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -DDEBUG -fsanitize=address")

# Find required packages
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)

# Python for training components
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)

# Optional: PyTorch C++ API (if available)
find_package(Torch QUIET)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/capsule_loader
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/hotswap_lanes
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/kernel_tutor
    ${CMAKE_CURRENT_SOURCE_DIR}/verification/static_analysis
    ${CMAKE_CURRENT_SOURCE_DIR}/../  # For Phase 2 MMM headers
    ${Python3_INCLUDE_DIRS}
)

# Source files for runtime components
set(CAPSULE_LOADER_SOURCES
    runtime/capsule_loader/capsule_loader.c
)

set(HOTSWAP_MANAGER_SOURCES
    runtime/hotswap_lanes/hotswap_manager.c
)

# Create capsule loader library
add_library(capsule_loader STATIC ${CAPSULE_LOADER_SOURCES})
target_link_libraries(capsule_loader 
    Threads::Threads
    ${CMAKE_DL_LIBS}
)

# Create hot-swap manager library
add_library(hotswap_manager STATIC ${HOTSWAP_MANAGER_SOURCES})
target_link_libraries(hotswap_manager 
    Threads::Threads
    ${CMAKE_DL_LIBS}
)

# Main Phase 3 integration executable
add_executable(phase3_ai_assembly_engineers
    integration/phase3_main.c
)

target_link_libraries(phase3_ai_assembly_engineers
    capsule_loader
    hotswap_manager
    Threads::Threads
    ${CMAKE_DL_LIBS}
    m  # Math library
)

# Test executables
add_executable(test_capsule_loader
    tests/test_capsule_loader.c
    ${CAPSULE_LOADER_SOURCES}
)

target_link_libraries(test_capsule_loader
    Threads::Threads
    ${CMAKE_DL_LIBS}
)

add_executable(test_hotswap_manager
    tests/test_hotswap_manager.c
    ${HOTSWAP_MANAGER_SOURCES}
)

target_link_libraries(test_hotswap_manager
    Threads::Threads
    ${CMAKE_DL_LIBS}
)

# Python training components (custom targets)
add_custom_target(training_datasets
    COMMAND ${Python3_EXECUTABLE} training/datasets/x86_assembly_dataset.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating training datasets"
)

add_custom_target(task_generator
    COMMAND ${Python3_EXECUTABLE} training/task_reward/task_generator.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running task generator"
)

add_custom_target(curriculum_manager
    COMMAND ${Python3_EXECUTABLE} training/curriculum/curriculum_manager.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running curriculum manager"
)

add_custom_target(learning_analytics
    COMMAND ${Python3_EXECUTABLE} training/analytics/learning_analytics.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running learning analytics"
)

add_custom_target(kernel_tutor
    COMMAND ${Python3_EXECUTABLE} runtime/kernel_tutor/kernel_tutor.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running kernel tutor"
)

add_custom_target(code_verifier
    COMMAND ${Python3_EXECUTABLE} verification/static_analysis/code_verifier.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running code verifier"
)

# PyTorch model training (if PyTorch is available)
if(Torch_FOUND)
    add_custom_target(assembly_transformer
        COMMAND ${Python3_EXECUTABLE} models/transformer/assembly_transformer.py
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Training assembly transformer model"
    )
    
    message(STATUS "PyTorch found - transformer training target available")
else()
    message(STATUS "PyTorch not found - transformer training will use CPU-only mode")
endif()

# Combined training target
add_custom_target(run_training_system
    DEPENDS training_datasets task_generator curriculum_manager learning_analytics
    COMMENT "Running complete training system"
)

# Combined verification target
add_custom_target(run_verification_system
    DEPENDS code_verifier
    COMMENT "Running verification system"
)

# Combined runtime target
add_custom_target(run_runtime_system
    DEPENDS kernel_tutor
    COMMENT "Running runtime system components"
)

# Installation
install(TARGETS phase3_ai_assembly_engineers
    RUNTIME DESTINATION bin
)

install(TARGETS capsule_loader hotswap_manager
    ARCHIVE DESTINATION lib
)

install(FILES
    runtime/capsule_loader/capsule_loader.h
    runtime/hotswap_lanes/hotswap_manager.h
    DESTINATION include/phase3
)

# Python components installation
install(DIRECTORY training/
    DESTINATION share/phase3/training
    FILES_MATCHING PATTERN "*.py"
)

install(DIRECTORY runtime/kernel_tutor/
    DESTINATION share/phase3/runtime
    FILES_MATCHING PATTERN "*.py"
)

install(DIRECTORY verification/
    DESTINATION share/phase3/verification
    FILES_MATCHING PATTERN "*.py"
)

install(DIRECTORY models/
    DESTINATION share/phase3/models
    FILES_MATCHING PATTERN "*.py"
)

# Documentation
install(FILES README.md
    DESTINATION share/doc/phase3
)

# Configuration files
install(FILES
    config/phase3_config.json.example
    config/training_config.json.example
    DESTINATION etc/phase3
    OPTIONAL
)

# Testing
enable_testing()

add_test(NAME test_capsule_loader
    COMMAND test_capsule_loader
)

add_test(NAME test_hotswap_manager
    COMMAND test_hotswap_manager
)

add_test(NAME test_training_datasets
    COMMAND ${Python3_EXECUTABLE} -m pytest training/datasets/
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(NAME test_verification_system
    COMMAND ${Python3_EXECUTABLE} -m pytest verification/
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# Custom test target for Python components
add_custom_target(test_python_components
    COMMAND ${Python3_EXECUTABLE} -m pytest -v
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running Python component tests"
)

# Performance benchmarks
add_custom_target(benchmark_capsule_loader
    COMMAND ./test_capsule_loader --benchmark
    DEPENDS test_capsule_loader
    COMMENT "Running capsule loader benchmarks"
)

add_custom_target(benchmark_hotswap_manager
    COMMAND ./test_hotswap_manager --benchmark
    DEPENDS test_hotswap_manager
    COMMENT "Running hot-swap manager benchmarks"
)

# Code analysis targets
add_custom_target(analyze
    COMMAND cppcheck --enable=all --std=c11 --std=c++17 ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running static analysis"
)

add_custom_target(format
    COMMAND find ${CMAKE_CURRENT_SOURCE_DIR} -name "*.c" -o -name "*.h" -o -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i
    COMMENT "Formatting source code"
)

# Memory check targets (if valgrind is available)
find_program(VALGRIND_EXECUTABLE valgrind)
if(VALGRIND_EXECUTABLE)
    add_custom_target(memcheck_capsule_loader
        COMMAND ${VALGRIND_EXECUTABLE} --tool=memcheck --leak-check=full ./test_capsule_loader
        DEPENDS test_capsule_loader
        COMMENT "Running memory check on capsule loader"
    )
    
    add_custom_target(memcheck_hotswap_manager
        COMMAND ${VALGRIND_EXECUTABLE} --tool=memcheck --leak-check=full ./test_hotswap_manager
        DEPENDS test_hotswap_manager
        COMMENT "Running memory check on hot-swap manager"
    )
endif()

# Package configuration
set(CPACK_PACKAGE_NAME "Phase3-AI-Assembly-Engineers")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_VENDOR "BDI Development Team")
set(CPACK_PACKAGE_CONTACT "bdi-dev@example.com")

set(CPACK_GENERATOR "TGZ;DEB")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libpthread-stubs0-dev, python3, python3-pip")

include(CPack)

# Print configuration summary
message(STATUS "")
message(STATUS "Phase 3: AI Assembly Engineers for BDI Configuration Summary")
message(STATUS "===========================================================")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "C Standard: C${CMAKE_C_STANDARD}")
message(STATUS "C++ Standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Python Executable: ${Python3_EXECUTABLE}")
message(STATUS "Python Version: ${Python3_VERSION}")
if(Torch_FOUND)
    message(STATUS "PyTorch: Found (${Torch_VERSION})")
else()
    message(STATUS "PyTorch: Not found (CPU-only mode)")
endif()
message(STATUS "")
message(STATUS "Available Targets:")
message(STATUS "  - phase3_ai_assembly_engineers: Main executable")
message(STATUS "  - run_training_system: Complete training system")
message(STATUS "  - run_verification_system: Verification system")
message(STATUS "  - run_runtime_system: Runtime system components")
message(STATUS "  - test: Run all tests")
message(STATUS "  - analyze: Static code analysis")
message(STATUS "  - format: Format source code")
message(STATUS "")

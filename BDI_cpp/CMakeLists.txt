cmake_minimum_required(VERSION 3.22)
project(BDI LANGUAGES CXX)

option(BDI_BUILD_TESTS "Build unit tests" ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BDI_SRC_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/BDI_cpp)

file(GLOB_RECURSE BDI_ALL_CPP
    ${BDI_SRC_ROOT}/**/*.cpp
    ${BDI_SRC_ROOT}/**/*.cc
    ${BDI_SRC_ROOT}/**/*.cxx
)

add_library(bdi STATIC ${BDI_ALL_CPP})
target_include_directories(bdi PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/BDI_cpp/BDIOS
    ${CMAKE_CURRENT_SOURCE_DIR}/BDI_cpp/Compiler/Backend
    ${CMAKE_CURRENT_SOURCE_DIR}/BDI_cpp/Compiler/IR
    ${CMAKE_CURRENT_SOURCE_DIR}/BDI_cpp/Compiler/Translations
    ${CMAKE_CURRENT_SOURCE_DIR}/BDI_cpp/Compiler/Typecheck
    ${CMAKE_CURRENT_SOURCE_DIR}/BDI_cpp/Core/Graph
    ${CMAKE_CURRENT_SOURCE_DIR}/BDI_cpp/Core/Payload
    ${CMAKE_CURRENT_SOURCE_DIR}/BDI_cpp/Core/Types
    ${CMAKE_CURRENT_SOURCE_DIR}/BDI_cpp/Devtools
    ${CMAKE_CURRENT_SOURCE_DIR}/BDI_cpp/Frontend/API
    ${CMAKE_CURRENT_SOURCE_DIR}/BDI_cpp/Frontend/DSL
    ${CMAKE_CURRENT_SOURCE_DIR}/BDI_cpp/HAL
    ${CMAKE_CURRENT_SOURCE_DIR}/BDI_cpp/Meta
    ${CMAKE_CURRENT_SOURCE_DIR}/BDI_cpp/Optimizer/Engine
    ${CMAKE_CURRENT_SOURCE_DIR}/BDI_cpp/Optimizer/Passes
    ${CMAKE_CURRENT_SOURCE_DIR}/BDI_cpp/Runtime/Learning
    ${CMAKE_CURRENT_SOURCE_DIR}/BDI_cpp/Runtime/Memory
    ${CMAKE_CURRENT_SOURCE_DIR}/BDI_cpp/Runtime/VM
)

add_executable(bdi_hello examples/hello_graph/main.cpp)
target_link_libraries(bdi_hello PRIVATE bdi)

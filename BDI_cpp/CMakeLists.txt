cmake_minimum_required(VERSION 3.22)
project(BDI LANGUAGES CXX)

option(BDI_BUILD_TESTS "Build unit tests" ON)
option(BDI_SANITIZE "Enable sanitizers (ASan/UBSan)" OFF)

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
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${BDI_SRC_ROOT}
)

if (BDI_SANITIZE)
  target_compile_options(bdi PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(bdi PUBLIC -fsanitize=address,undefined)
endif()

add_executable(bdi_hello examples/hello_graph/main.cpp)
target_link_libraries(bdi_hello PRIVATE bdi)

cmake_minimum_required(VERSION 3.14)
project(CRTIME CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")

add_subdirectory(src)
add_executable(crtime src/main.cpp)
target_link_libraries(crtime PRIVATE libcrtime)

# wasmtime
add_subdirectory(third_party/wasmtime/crates/c-api)

# cxxopts
add_subdirectory(third_party/cxxopts)

# yalantinglibs
add_subdirectory(third_party/yalantinglibs)

# spdlog
add_subdirectory(third_party/spdlog)

# fmtlib
add_subdirectory(third_party/fmt)

# tests
option(ENABLE_TESTS "Enable unit tests" ON)
if (ENABLE_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
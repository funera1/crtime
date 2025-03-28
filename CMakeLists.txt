cmake_minimum_required(VERSION 3.14)
project(CRTIME CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")

add_executable(crtime main.cpp regs.cpp stack.cpp)

# wasmtime
add_subdirectory(third_party/wasmtime/crates/c-api)
target_link_libraries(crtime PUBLIC wasmtime)
target_include_directories(crtime PUBLIC wasmtime)

# cxxopts
add_subdirectory(third_party/cxxopts)
target_link_libraries(crtime PUBLIC cxxopts)
target_include_directories(crtime PUBLIC cxxopts)

# yalantinglibs
add_subdirectory(third_party/yalantinglibs)
target_link_libraries(crtime PUBLIC yalantinglibs::yalantinglibs)
target_include_directories(crtime PUBLIC yalantinglibs/include)

# spdlog
add_subdirectory(third_party/spdlog)
target_link_libraries(crtime PUBLIC spdlog::spdlog)
# target_include_directories(crtime PUBLIC yalantinglibs/include)
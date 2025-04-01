cmake_minimum_required(VERSION 3.14)
project(CRTIME CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")

add_library(libcrtime STATIC 
    src/regs.cpp 
    src/stack.cpp 
    src/vmcxt.cpp 
    src/utils.cpp 
    src/signal_handler.cpp 
    src/option.cpp
)

add_executable(crtime src/main.cpp)
target_link_libraries(crtime PRIVATE libcrtime)
target_include_directories(libcrtime PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# wasmtime
add_subdirectory(third_party/wasmtime/crates/c-api)
target_link_libraries(libcrtime PUBLIC wasmtime)

# cxxopts
add_subdirectory(third_party/cxxopts)
target_link_libraries(libcrtime PUBLIC cxxopts)

# yalantinglibs
add_subdirectory(third_party/yalantinglibs)
target_link_libraries(libcrtime PUBLIC yalantinglibs::yalantinglibs)

# spdlog
add_subdirectory(third_party/spdlog)
target_link_libraries(libcrtime PUBLIC spdlog::spdlog)

# fmtlib
add_subdirectory(third_party/fmt)
target_link_libraries(libcrtime PUBLIC fmt::fmt)

# tests
option(ENABLE_TESTS "Enable unit tests" ON)
if (ENABLE_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
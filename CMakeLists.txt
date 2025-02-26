cmake_minimum_required(VERSION 3.14)
project(CWTIME C)

add_subdirectory(third_party/wasmtime/crates/c-api)
# ${CMAKE_CURRENT_BINARY_DIR}/wasmtime)

add_executable(cwtime main.c)

target_link_libraries(cwtime PUBLIC wasmtime)
target_include_directories(cwtime PUBLIC wasmtime)

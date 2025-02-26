cmake_minimum_required(VERSION 3.14)
project(CWTIME CXX)

add_executable(cwtime main.cpp)

# wasmtime
add_subdirectory(third_party/wasmtime/crates/c-api)
target_link_libraries(cwtime PUBLIC wasmtime)
target_include_directories(cwtime PUBLIC wasmtime)

# cxxopts
add_subdirectory(third_party/cxxopts)
target_link_libraries(cwtime PUBLIC cxxopts)
target_include_directories(cwtime PUBLIC cxxopts)

# yalantinglibs
add_subdirectory(third_party/yalantinglibs)
target_link_libraries(cwtime PUBLIC yalantinglibs::yalantinglibs)
target_include_directories(cwtime PUBLIC yalantinglibs/include)
# target_compile_features(cwtime PRIVATE cxx_std_20)
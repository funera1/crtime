cmake_minimum_required(VERSION 3.14)
project(CRTIME CXX)

add_executable(crtime main.cpp regs.cpp)

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
# target_compile_features(crtime PRIVATE cxx_std_20)

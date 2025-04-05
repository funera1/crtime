add_library(libcrtime STATIC
    regs.cpp
    stack.cpp
    vmcxt.cpp
    utils.cpp
    signal_handler.cpp
    option.cpp
    restore.cpp
)

# ヘッダーファイルのインクルード
target_include_directories(libcrtime PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# 必要なライブラリをリンク
target_link_libraries(libcrtime PUBLIC wasmtime cxxopts yalantinglibs::yalantinglibs spdlog::spdlog fmt::fmt)
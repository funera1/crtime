#include "option.h"
#include "vmcxt.h"
#include "signal_handler.h"
#include "utils.h"
#include <spdlog/spdlog.h>
#include <cxxopts.hpp>

int main(int argc, char* argv[]) {
    spdlog::set_level(spdlog::level::trace);
    spdlog::info("Hello spdlog");

    // コマンドラインオプションの解析
    Option option = parse_options(argc, argv);
    if (!option.is_valid()) return 0;

    // SIGTRAP ハンドラ登録
    register_sigtrap();

    // init loggger for wasmtime
    wasmtime_config_init_logger();

    // WASM 実行環境の初期化
    VMCxt vm(option);
    if (!vm.initialize()) {
        spdlog::error("Failed to initialize VM");
        return -1;
    }

    // WASM モジュールの実行
    if (!vm.execute()) {
        spdlog::error("Failed to execute WASM");
        return -1;
    }

    return 0;
}
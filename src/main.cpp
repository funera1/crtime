#include "option.h"
#include "vmcxt.h"
#include "signal_handler.h"
#include "utils.h"
#include "restore.h"
#include <spdlog/spdlog.h>
#include <cxxopts.hpp>
#include <wasmtime.h>
#include <malloc.h>

int main(int argc, char* argv[]) {
    spdlog::set_level(spdlog::level::trace);
    spdlog::info("Hello spdlog");

    // コマンドラインオプションの解析
    Option option = parse_options(argc, argv);
    if (!option.is_valid()) return 0;

    // init loggger for wasmtime
    wasmtime_config_init_logger();

    if (option.is_explore) {
        wasm_config_t *config = wasm_config_new();
        wasmtime_config_strategy_set(config, WASMTIME_STRATEGY_WINCH);
        
        // restore
        set_restore_info(config, option.restore_opt);

        wasmtime_explore(config, option.path.c_str());
        return 0;
    } else {
        // WASM 実行環境の初期化
        VMCxt vm(option);
        if (!vm.initialize()) {
            spdlog::error("Failed to initialize VM");
            return -1;
        }
        
        // Address Mapの取得
        AddressMap addr_map;
        if (auto ret = vm.get_address_map(); ret.has_value()) {
            addr_map = ret.value();
        } else {
            spdlog::error("Failed to get address map");
            return -1;
        }

        // SIGTRAP ハンドラ登録
        register_sigusr1(&addr_map, sigusr1_handler);
        register_sigtrap(&vm, sigtrap_handler, global_vm_setter);

        // WASM モジュールの実行
        if (!vm.execute()) {
            spdlog::error("Failed to execute WASM");
            return -1;
        }

        return 0;
    }

    return 0;
}
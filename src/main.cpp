#include "option.h"
#include "vmcxt.h"
#include "signal_handler.h"
#include "utils.h"
#include "restore.h"
#include <spdlog/spdlog.h>
#include <cxxopts.hpp>
#include <wasmtime.h>
#include <malloc.h>

int exec() {
// 初期化
    wasm_engine_t* engine = wasm_engine_new();
    wasmtime_store_t* store = wasmtime_store_new(engine, NULL, NULL);
    wasmtime_context_t* context = wasmtime_store_context(store);

    // Wasmバイナリを読み込み
    FILE* file = fopen("example.wasm", "rb");
    fseek(file, 0, SEEK_END);
    size_t wasm_size = ftell(file);
    rewind(file);
    uint8_t* wasm_bytes = (uint8_t *)malloc(wasm_size);
    fread(wasm_bytes, 1, wasm_size, file);
    fclose(file);

    // モジュールを作成
    wasmtime_module_t* module = NULL;
    wasmtime_error_t* error = wasmtime_module_new(engine, wasm_bytes, wasm_size, &module);
    free(wasm_bytes);
    if (error != NULL) {
        // wasmtime_error_message(error, &(wasm_name_t){0});
        printf("Failed to compile module\n");
        exit(1);
    }
    wasm_trap_t* trap = NULL;

    // インスタンス作成
    wasmtime_instance_t instance;
    error = wasmtime_instance_new(context, module, NULL, 0, &instance, &trap);
    if (error || trap) {
        printf("Failed to instantiate module\n");
        exit(1);
    }

    // メモリを取得
    wasmtime_extern_t item;
    bool ok = wasmtime_instance_export_get(context, &instance, "memory", strlen("memory"), &item);
    if (!ok || item.kind != WASMTIME_EXTERN_MEMORY) {
        printf("Failed to get memory\n");
        exit(1);
    }
    wasmtime_memory_t memory = item.of.memory;

    // メモリに書き込み
    uint32_t value = 42;
    uint8_t* mem_data = wasmtime_memory_data(context, &memory);
    memcpy(mem_data, &value, sizeof(value));


    
    // 方法B: linkerを使う
    wasmtime_linker_t* linker = wasmtime_linker_new(engine);
    wasmtime_linker_allow_unknown_exports(linker, true);
    wasmtime_linker_define_instance(linker, context, "", 0, &instance);

    wasmtime_func_t func;
    error = wasmtime_linker_get_default(linker, context, "", 0, &func);
    if (error != NULL) {
        exit_with_error("failed to locate default export for module", error, NULL);
    }

    wasmtime_val_t result;
    wasmtime_val_t params[0];
    error = wasmtime_func_call(context, &func, params, 0, &result, 1, &trap);
    if (error || trap) {
        printf("Failed to call function\n");
        exit(1);
    }

    printf("WASM returned: %d\n", result.of.i32); // → 42
                                                  //
    return 0;
}

int exec2() {
    // 初期化
    wasm_engine_t* engine = wasm_engine_new();
    wasmtime_store_t* store = wasmtime_store_new(engine, NULL, NULL);
    wasmtime_context_t* context = wasmtime_store_context(store);
    wasm_trap_t* trap = NULL;

    // Wasmバイナリを読み込み
    FILE* file = fopen("example.wasm", "rb");
    fseek(file, 0, SEEK_END);
    size_t wasm_size = ftell(file);
    rewind(file);
    uint8_t* wasm_bytes = (uint8_t *)malloc(wasm_size);
    fread(wasm_bytes, 1, wasm_size, file);
    fclose(file);

    // モジュールを作成
    wasmtime_module_t* module = NULL;
    wasmtime_error_t* error = wasmtime_module_new(engine, wasm_bytes, wasm_size, &module);
    free(wasm_bytes);
    if (error != NULL) {
        printf("Failed to compile module\n");
        exit(1);
    }

    // インスタンス作成
    wasmtime_instance_t instance;
    error = wasmtime_instance_new(context, module, NULL, 0, &instance, &trap);
    if (error || trap) {
        printf("Failed to instantiate module\n");
        exit(1);
    }

    // メモリを取得
    wasmtime_extern_t item;
    bool ok = wasmtime_instance_export_get(context, &instance, "memory", strlen("memory"), &item);
    if (!ok || item.kind != WASMTIME_EXTERN_MEMORY) {
        printf("Failed to get memory\n");
        exit(1);
    }
    wasmtime_memory_t memory = item.of.memory;

    // ホストがメモリに 42 を書き込む
    uint32_t value = 42;
    uint8_t* mem_data = wasmtime_memory_data(context, &memory);
    memcpy(mem_data, &value, sizeof(value));

    // Wasm 関数呼び出し
    wasmtime_extern_t func_item;
    ok = wasmtime_instance_export_get(context, &instance, "_start", strlen("_start"), &func_item);
    if (!ok || func_item.kind != WASMTIME_EXTERN_FUNC) {
        printf("Failed to get function\n");
        exit(1);
    }

    wasmtime_val_t result;
    wasmtime_val_t params[0];
    error = wasmtime_func_call(context, &func_item.of.func, params, 0, &result, 1, &trap);
    if (error || trap) {
        printf("Failed to call function\n");
        exit(1);
    }

    printf("WASM returned: %d\n", result.of.i32); // → 42
    return 0;
}

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

        // SIGTRAP ハンドラ登録
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
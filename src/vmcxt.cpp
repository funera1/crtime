#include "vmcxt.h"
#include "utils.h"
#include <spdlog/spdlog.h>
#include <fstream>
#include <cassert>

VMCxt::VMCxt(const Option& opt) : option(opt), engine(nullptr), store(nullptr), linker(nullptr), module(nullptr), context(nullptr), trap(nullptr) {}

VMCxt::~VMCxt() {
    if (module) wasmtime_module_delete(module);
    if (store) wasmtime_store_delete(store);
    if (engine) wasm_engine_delete(engine);
}

bool VMCxt::initialize() {
    wasm_config_t* config = wasm_config_new();
    wasmtime_config_strategy_set(config, WASMTIME_STRATEGY_WINCH);

    engine = wasm_engine_new_with_config(config);
    store = wasmtime_store_new(engine, NULL, NULL);
    linker = wasmtime_linker_new(engine);
    context = wasmtime_store_context(store);
    if (!engine || !store || !linker) return false;

    wasmtime_linker_allow_unknown_exports(linker, true);
    return true;
}

bool VMCxt::execute() {
    return true;
    // Create a linker with WASI functions defined
    // wasmtime_error_t *error = wasmtime_linker_define_wasi(linker);
    // if (error != NULL) {
    //     exit_with_error("failed to link wasi", error, NULL);
    // }

    // // Load our input file to parse it next
    // wasm_byte_vec_t wasm = load_wasm_file(file_name);

    // // Compile our modules
    // error = wasmtime_module_new(vm->engine, (uint8_t *)wasm.data, wasm.size, &vm->module);
    // if (!vm->module)
    //     exit_with_error("failed to compile module", error, NULL);
    // wasm_byte_vec_delete(&wasm);

    // // Instantiate wasi
    // instantiate_wasi(vm->context, vm->trap);

    // // Instantiate the module
    // error = wasmtime_linker_module(vm->linker, vm->context, "", 0, vm->module);
    // if (error != NULL)
    //     exit_with_error("failed to instantiate module", error, NULL);

    // // Run it.
    // wasmtime_func_t func;
    // error = wasmtime_linker_get_default(vm->linker, vm->context, "", 0, &func);
    // if (error != NULL)
    //     exit_with_error("failed to locate default export for module", error, NULL);

    // error = wasmtime_func_call(vm->context, &func, NULL, 0, NULL, 0, &vm->trap);
    // if (error != NULL || vm->trap != NULL)
    //     exit_with_error("error calling default export", error, vm->trap);
    // return true;
}
#include "vmcxt.h"
#include "utils.h"
#include "stack.h"
#include <spdlog/spdlog.h>
#include <fstream>
#include <cassert>
#include <format>

using namespace std;

VMCxt::VMCxt(const Option& opt) : option(opt), engine(nullptr), store(nullptr), linker(nullptr), module(nullptr), context(nullptr), trap(nullptr) {}

VMCxt::~VMCxt() {
    if (module) wasmtime_module_delete(module);
    if (store) wasmtime_store_delete(store);
    if (engine) wasm_engine_delete(engine);
}

void set_restore_info(wasm_config_t* config) {
    wasmtime_config_set_restore_info(config, true);
}

bool VMCxt::initialize() {
    wasm_config_t* config = wasm_config_new();
    wasmtime_config_strategy_set(config, WASMTIME_STRATEGY_WINCH);
    
    // restore
    set_restore_info(config);

    engine = wasm_engine_new_with_config(config);
    store = wasmtime_store_new(engine, NULL, NULL);
    linker = wasmtime_linker_new(engine);
    context = wasmtime_store_context(store);
    if (!engine || !store || !linker || !context) return false;

    // linker setting
    wasmtime_linker_allow_unknown_exports(linker, true);

    // Create a linker with WASI functions defined
    wasmtime_error_t *error = wasmtime_linker_define_wasi(linker);
    if (error != NULL) {
        exit_with_error("failed to link wasi", error, NULL);
    }
    
    // new module
    // Compile our modules
    wasm_byte_vec_t wasm = option.wasm;
    error = wasmtime_module_new(engine, (uint8_t *)wasm.data, wasm.size, &module);
    if (!module) {
        exit_with_error("failed to compile module", error, NULL);
    }
    wasm_byte_vec_delete(&wasm);

    // Instantiate wasi
    instantiate_wasi(context, trap);

    // Instantiate the module
    error = wasmtime_linker_module(linker, context, "", 0, module);
    if (error != NULL) {
        exit_with_error("failed to instantiate module", error, NULL);
    }

    return true;
}

bool VMCxt::execute() {
    wasmtime_error_t *error;

    // Run it.
    wasmtime_func_t func;
    error = wasmtime_linker_get_default(linker, context, "", 0, &func);
    if (error != NULL) {
        exit_with_error("failed to locate default export for module", error, NULL);
    }

    // TODO: 自由に引数/返り値を扱えるようにする(現在は両方0しかだめ)
    error = wasmtime_func_call(context, &func, NULL, 0, NULL, 0, &trap);
    if (error != NULL || trap != NULL) {
        exit_with_error("error calling default export", error, trap);
    }
    return true;
}


wasmtime_instance_t VMCxt::get_instance() {
    wasmtime_instance_t instance;
    wasmtime_error_t *error = wasmtime_linker_instantiate(linker, context, module, &instance, &trap);
    if (error != NULL || trap != NULL) {
      exit_with_error("failed to get module module", error, trap);
    }

    return instance;
}

optional<AddressMap> VMCxt::get_address_map() {
    // moduleのnullチェック
    if (module == NULL)
    {
        spdlog::error("module is null");
        return nullopt;
    }

    // wasmtimeからaddress_mapをもらう
    wasmtime_addrmap_entry_t *data;
    size_t len;
    uintptr_t base_addr;
    wasmtime_module_address_map(module, &data, &len, &base_addr);

    vector<wasmtime_addrmap_entry_t> address_map(data, data + len);

    // debug: print address map
    if (option.is_print_addrmap)
    {
        string s = "(code offs, wasm offs): [";
        for (auto ad : address_map)
            s += format(" ({}, {}) ", ad.code_offset, ad.wasm_offset);
        s += "]";
        spdlog::debug("{:s}", s);
    }

    return AddressMap(base_addr, address_map);
}

vector<wasmtime_ssmap_entry_t> VMCxt::get_stack_size_maps() {
    if (module == NULL) {
      spdlog::error("module is NULL");
    }

    // stack_size_mapsを取得
    wasmtime_ssmap_entry_t *data;
    size_t len;
    wasmtime_module_stack_size_maps(module, &data, &len);
    spdlog::info("return wasmtime_module_stack_size_maps");

    vector<wasmtime_ssmap_entry_t> stack_size_map(data, data+len);

    // debug: print address map
    if (option.is_print_ssmap) {
      string s = "(wasm offs, stack size): [";
      for (auto iter : stack_size_map) s += format(" ({}, {}) ", iter.wasm_offset, iter.stack_size);
      s += "]";
      spdlog::debug("{:s}", s);
    }

    return stack_size_map;
}

std::optional<vector<uint8_t>> VMCxt::get_memory() {
  wasmtime_instance_t instance = get_instance();
  wasmtime_extern_t export_;
  
  std::string name = "memory";
  bool ok = wasmtime_instance_export_get(context, &instance, name.c_str(), name.size(), &export_);
  if (!ok || export_.kind != WASMTIME_EXTERN_MEMORY) {
    spdlog::error("failed to get memory export");
    return nullopt;
  }

  wasmtime_memory_t memory = export_.of.memory;
  uint8_t* data = wasmtime_memory_data(context, &memory);
  size_t size = wasmtime_memory_data_size(context, &memory);
  return std::vector<uint8_t>(data, data+size);
}

// TODO: 実装できてない, globalを事前に特定の名前でexportしないといけない設計になっていて良くない
std::vector<global_t> VMCxt::get_globals() {
  wasmtime_instance_t instance = get_instance();
  wasmtime_extern_t export_;

  std::vector<global_t> globals;
  
  // TODO: 1文字しか対応できてない
  std::string base_name = "global_";
  for (int i = 0; i < 10; i++) {
    std::string name = base_name + to_string(i);
    bool ok = wasmtime_instance_export_get(context, &instance, name.c_str(), name.size(), &export_);
    // TODO: エラーとglobalが存在するかの判定ができるようにする
    if (!ok) {
      break;
    }

    wasmtime_global_t global = export_.of.global;
    wasmtime_val_t value;
    wasmtime_global_get(context, &global, &value);

    global_t g;
    g.kind = value.kind;
    switch(value.kind) {
      case WASMTIME_I32:
        g.value = value.of.i32;
        break;
      case WASMTIME_F32:
        g.value = value.of.f32;
        break;
      case WASMTIME_I64:
        g.value = value.of.i64;
        break;
      case WASMTIME_F64:
        g.value = value.of.f64;
        break;
      default:
        break;
    }
    globals.push_back(g);
  }

  return globals;
}
#include "vmcxt.h"
#include "utils.h"
#include "stack.h"
#include "restore.h"
#include <spdlog/spdlog.h>
#include <fstream>
#include <cassert>
#include <format>
#include <iostream>
#include <fmt/ranges.h>

using namespace std;

VMCxt::VMCxt(const Option& opt) : option(opt), engine(nullptr), store(nullptr), linker(nullptr), module(nullptr), context(nullptr), trap(nullptr) {}

VMCxt::~VMCxt() {
    if (module) wasmtime_module_delete(module);
    if (store) wasmtime_store_delete(store);
    if (engine) wasm_engine_delete(engine);
}

bool VMCxt::initialize() {
    config = wasm_config_new();
    wasmtime_config_strategy_set(config, WASMTIME_STRATEGY_WINCH);
    
    // restore
    set_restore_info(config, option.restore_opt);

    engine = wasm_engine_new_with_config(config);
    store = wasmtime_store_new(engine, NULL, NULL);
    context = wasmtime_store_context(store);
    if (!engine || !store || !context) return false;

    wasmtime_error_t *error;
    // new module
    // Compile our modules
    wasm_byte_vec_t wasm = option.wasm;
    error = wasmtime_module_new(engine, (uint8_t *)wasm.data, wasm.size, &module);
    if (!module) {
        exit_with_error("failed to compile module", error, NULL);
    }
    wasm_byte_vec_delete(&wasm);
    spdlog::info("new module");
    
    // new instance
    // NOTE: memoryの操作はinstanceを介してやる
    // wasmtime_instance_t instance;
    error = wasmtime_instance_new(context, module, NULL, 0, &instance, &trap);
    if (error || trap) {
        printf("Failed to instantiate module\n");
        exit(1);
    }
    spdlog::info("new instance");

    // restore memory and global
    if (option.restore_opt.is_restore) {
      restore_memory(&instance);
    }

    // linker setting
    linker = wasmtime_linker_new(engine);
    wasmtime_linker_allow_unknown_exports(linker, true);
    spdlog::info("new linker");

    // Instantiate the module
    // error = wasmtime_linker_module(linker, context, "", 0, module);
    error = wasmtime_linker_define_instance(linker, context, "", 0, &instance);
    if (error != NULL) {
        exit_with_error("failed to instantiate module", error, NULL);
    }
    spdlog::info("linker define instance");

    // Create a linker with WASI functions defined
    error = wasmtime_linker_define_wasi(linker);
    if (error != NULL) {
        exit_with_error("failed to link wasi", error, NULL);
    }
    spdlog::info("linker define wasi");

    return true;
}

bool VMCxt::explore() {
  wasmtime_explore(config, option.path.c_str());
  return true;
}

optional<vector<wasmtime_val_t>> VMCxt::execute() {
    wasmtime_error_t *error;

    // Run it.
    wasmtime_func_t func;
    error = wasmtime_linker_get_default(linker, context, "", 0, &func);
    if (error != NULL) {
        exit_with_error("failed to locate default export for module", error, NULL);
    }
    spdlog::info("get entry func");
    
    // 関数の型を取得
    wasm_functype_t* ftype = wasmtime_func_type(context, &func);
    const wasm_valtype_vec_t* params = wasm_functype_params(ftype);
    const wasm_valtype_vec_t* results = wasm_functype_results(ftype);

    // TODO: 自由に引数/返り値を扱えるようにする(現在は両方0しかだめ)
    wasmtime_val_t* params_vec = wasmtime_val_new(params);
    wasmtime_val_t* results_vec = wasmtime_val_new(results);
    spdlog::info("func call");
    error = wasmtime_func_call(context, &func, params_vec, params->size, results_vec, results->size, &trap);
    if (error != NULL || trap != NULL) {
        exit_with_error("error calling default export", error, trap);
    }
    
    // 返り値がある場合
    if (results->size > 0) {
      for (int i = 0; i < results->size; i++) {
        wasmtime_val_t val = results_vec[i];
        switch (val.kind) {
          case WASM_I32: cout << val.of.i32 << endl; break;
          case WASM_F32: cout << val.of.f32 << endl; break;
          case WASM_I64: cout << val.of.i64 << endl; break;
          case WASM_F64: cout << val.of.f64 << endl; break;
          default: spdlog::error("unsupported type"); break;
        }
      }
    }
    
    return vector<wasmtime_val_t>(results_vec, results_vec+results->size);
}


wasmtime_instance_t VMCxt::get_instance() {
    wasmtime_instance_t instance;
    wasmtime_error_t *error = wasmtime_linker_instantiate(linker, context, module, &instance, &trap);
    if (error != NULL || trap != NULL) {
      exit_with_error("failed to get module module", error, trap);
    }

    return instance;
}

wasmtime_context_t* VMCxt::get_context() {
  return context;
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

std::optional<Memory> VMCxt::get_memory() {
  // wasmtime_instance_t instance = get_instance();
  //
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
  return Memory{memory, data, size};
}

std::optional<size_t> VMCxt::get_memsize() {
  wasmtime_instance_t instance = get_instance();
  wasmtime_extern_t export_;
  
  std::string name = "memory";
  bool ok = wasmtime_instance_export_get(context, &instance, name.c_str(), name.size(), &export_);
  if (!ok || export_.kind != WASMTIME_EXTERN_MEMORY) {
    spdlog::error("failed to get memory export");
    return nullopt;
  }
  wasmtime_memory_t memory = export_.of.memory;

  return wasmtime_memory_data_size(context, &memory);
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

enum WasmValType {
  I32 = 0x7F,
  I64 = 0x7E,
  F32 = 0x7D,
  F64 = 0x7C,
};

Locals VMCxt::get_locals(uintptr_t rsp, size_t index) {
  // local infoを取得
  wasmtime_local_info_t *data;
  size_t len;
  wasmtime_module_local_info(module, index, &data, &len);
  vector<wasmtime_local_info_t> local_info(data, data+len);
  
  // localを取得
  vector<uint32_t> locals(len);
  vector<uint8_t> types(len);
  for (int i = 0; i < len; i++) {
    wasmtime_local_info_t info = local_info[i];
    types[i] = info.ty;
    spdlog::debug("{}th offset: {}", i, info.offset);
    switch (info.ty) {
      case WasmValType::I32:
      case WasmValType::F32:
        locals[i] = *(uint32_t *)(rsp + info.offset);
        break;
      case WasmValType::I64:
      case WasmValType::F64:
        spdlog::error("Not support i64 and f64");
        break;
      default:
        spdlog::error("Not support i128 and more");
        break;
    }
  }
  
  return Locals(types, locals);
}

void VMCxt::restore_memory(wasmtime_instance_t *instance) {
    // メモリを取得
    wasmtime_extern_t item;
    bool ok = wasmtime_instance_export_get(context, instance, "memory", strlen("memory"), &item);
    if (!ok || item.kind != WASMTIME_EXTERN_MEMORY) {
        printf("Failed to get memory\n");
        exit(1);
    }
    wasmtime_memory_t memory = item.of.memory;

    // ホストがメモリに 42 を書き込む
    uint32_t value = 42;
    uint8_t* mem_data = wasmtime_memory_data(context, &memory);
    memcpy(mem_data, &value, sizeof(value));
  
  // if (!wasmtime_memory_write(context, &mem.memory, 0, mem.data, mem.size)) {
  // if (!wasmtime_memory_write(context, &mem.memory, 0, newm.data(), newm.size())) {
  //   spdlog::error("failed to restore memory");
  // }
}
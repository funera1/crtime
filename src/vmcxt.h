#pragma once
#include <string>
#include <vector>
#include <optional>
#include <wasmtime.h>
#include <wasm.h>
#include "option.h"

typedef struct global {
  int kind;
  int64_t value;
} global_t;

struct globals {
  // vector<wasmtime_val_t> globals;
  std::vector<global_t> globals;
};

class VMCxt {
public:
    VMCxt(const Option& opt);
    ~VMCxt();
    
    bool initialize();
    bool execute();
    
    std::optional<std::vector<uint8_t>> get_memory();
    std::vector<int> get_stack();
    std::vector<global_t> get_globals();
    
private:
    wasm_engine_t *engine;
    wasmtime_store_t *store;
    wasmtime_linker_t *linker;
    wasmtime_module_t *module; 
    wasmtime_context_t *context;
    wasm_trap_t *trap;
    Option option;
};
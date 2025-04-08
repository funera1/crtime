#pragma once
#include <string>
#include <vector>
#include <optional>
#include <wasmtime.h>
#include <wasm.h>
#include <signal.h>
#include "option.h"
#include "stack.h"
#include "memory.h"

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
    bool explore();
    optional<vector<wasmtime_val_t>> execute();
    
    // privateでもいい
    wasmtime_instance_t get_instance();
    wasmtime_context_t* get_context();
    optional<AddressMap> get_address_map();
    vector<wasmtime_ssmap_entry_t> get_stack_size_maps();

    std::optional<Memory> get_memory();
    std::optional<size_t> get_memsize();
    std::vector<int> get_stack();
    std::vector<global_t> get_globals();
    Locals get_locals(uintptr_t rsp, size_t index);

    void restore_memory(wasmtime_instance_t *instnace);
    
private:
    wasm_engine_t *engine;
    wasm_config_t *config;
    wasmtime_store_t *store;
    wasmtime_linker_t *linker;
    wasmtime_module_t *module; 
    wasmtime_context_t *context;
    wasm_trap_t *trap;
    Option option;
};
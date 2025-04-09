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
#include <ylt/struct_pack.hpp>

typedef struct global {
  int kind;
  std::string name;
  int64_t value;
} global_t;

struct Globals {
  // vector<wasmtime_val_t> globals;
  std::vector<global_t> globals;

  YLT_REFL(Globals, globals);
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
    vector<uint8_t> read_file_to_buffer(const std::string& file_path);
    void grow_memory_if_needed(wasmtime_memory_t& memory, size_t required_size);

    std::optional<Memory> get_memory();
    std::optional<size_t> get_memsize();
    std::vector<int> get_stack();
    Globals get_globals();
    Locals get_locals(uintptr_t rsp, size_t index);

    void restore_memory(wasmtime_instance_t *instnace);
    void restore_globals(wasmtime_instance_t *instnace);
    
private:
    wasm_engine_t *engine;
    wasm_config_t *config;
    wasmtime_store_t *store;
    wasmtime_linker_t *linker;
    wasmtime_module_t *module; 
    wasmtime_context_t *context;
    wasmtime_instance_t instance;
    wasm_trap_t *trap;
    Option option;
};
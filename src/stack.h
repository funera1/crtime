#ifndef STACK_H
#define STACK_H

#include <vector>
#include <spdlog/spdlog.h>
#include <wasmtime.h>
#include <ylt/struct_pack.hpp>

using namespace std;

class AddressMap {

public:
    uintptr_t base_address;
    std::vector<wasmtime_addrmap_entry_t> addrmap;

    AddressMap(uintptr_t base_addr, std::vector<wasmtime_addrmap_entry_t> addrmap) : base_address(base_addr), addrmap(addrmap){}

    uint32_t get_wasm_offset(uintptr_t rip) {
        // sigtrap handlerが呼び出されるときには、int3から1つ進んでいるため,-1する
        uint32_t pc_code_offset = rip - base_address - 1;
        for (auto addr : addrmap) {
            if (addr.code_offset == pc_code_offset) {
              return addr.wasm_offset;
            }
        }
        spdlog::error("Not found target wasm offset in address map");
        return 0xdeadbeaf;
    }
};

struct Stack {
    // vector<stack_entry_t> stack;
    vector<uint32_t> metadata;
    vector<uint32_t> values;
    
    Stack() = default;
    Stack(vector<uint32_t> m, vector<uint32_t> v): metadata(m), values(v) {};
    
    wasmtime_stack_t wasmtime_stack();

    // YLT_REFLマクロを使ってメンバを登録
    YLT_REFL(Stack, metadata, values);
};

struct Locals {
  vector<uint8_t> types;
  vector<uint32_t> values;
  
  Locals() = default;
  Locals(vector<uint8_t> ty, vector<uint32_t> vals): types(ty), values(vals) {}

  // YLT_REFLマクロを使ってメンバを登録
  YLT_REFL(Locals, types, values);
};

Stack reconstruct_stack(vector<uintptr_t> &regs, std::vector<wasmtime_ssmap_entry_t> &stack_size_map, uint32_t pc);
vector<uint32_t> get_stack_vals(Stack st);

#endif
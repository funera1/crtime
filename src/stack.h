#ifndef STACK_H
#define STACK_H

#include <vector>
#include <spdlog/spdlog.h>
#include <wasmtime.h>

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

struct stack_entry_t {
  uint8_t reg_id;
  uint32_t value;
  
  stack_entry_t() = default;
  stack_entry_t(uint8_t id, uint32_t v) : reg_id(id), value(v) {};
};

vector<stack_entry_t> reconstruct_stack(vector<uintptr_t> &regs, std::vector<wasmtime_ssmap_entry_t> &stack_size_map, uint32_t pc);
vector<uint32_t> get_stack_vals(vector<stack_entry_t> stack);

#endif
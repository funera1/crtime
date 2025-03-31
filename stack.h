#ifndef STACK_H
#define STACK_H

#include <vector>

using namespace std;

class AddressMap {

public:
    uintptr_t base_address;
    vector<wasmtime_addrmap_entry_t> addrmap;

    AddressMap(uintptr_t base_addr, vector<wasmtime_addrmap_entry_t> addrmap) : base_address(base_addr), addrmap(addrmap){}

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
vector<int> reconstruct_stack(vector<uintptr_t> &regs, vector<wasmtime_ssmap_entry_t> &stack_size_map, uint32_t pc);

#endif
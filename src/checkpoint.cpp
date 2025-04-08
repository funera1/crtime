#include <filesystem>
#include <fstream>
#include <vmcxt.h>
#include <stack.h>
#include <regs.h>
#include <checkpoint.h>
#include <ylt/struct_pack.hpp>
#include <fmt/ranges.h>

using namespace std;
namespace fs = std::filesystem;

void Checkpointer::checkpoint_stack(uint32_t pc) {
    vector<wasmtime_ssmap_entry_t> stack_size_maps = vm->get_stack_size_maps();
    spdlog::info("Get stack size map");
    Stack stack = reconstruct_stack(regs, stack_size_maps, pc);
    spdlog::debug("stack: [{}]", fmt::join(stack.values, ", "));
    spdlog::info("Reconstruct stack");
    
    vector<char> buffer = struct_pack::serialize(stack);
    if (!write_binary("wasm_stack.img", (uint8_t *)buffer.data(), buffer.size())) {
      spdlog::error("failed to checkpoint stack");
    }
    spdlog::info("Checkpoint stack");
}

void Checkpointer::checkpoint_locals(uint32_t pc) {
    Locals locals = vm->get_locals(regs[ENC_RSP], 0);
    spdlog::debug("locals: [{}]", fmt::join(locals.values, ", "));

    vector<char> buffer = struct_pack::serialize(locals);
    if (!write_binary("wasm_locals.img", (uint8_t *)buffer.data(), buffer.size())) {
      spdlog::error("failed to checkpoint locals");
    }
    spdlog::info("Checkpoint locals");
}

uint32_t Checkpointer::checkpoint_pc() {
    auto ret = vm->get_address_map();
    if (!ret.has_value()) {
        spdlog::error("failed to get address map");
        return -1;
    }
    AddressMap addrmap = ret.value();
    spdlog::info("Get address map");

    uint32_t pc = addrmap.get_wasm_offset(regs[ENC_RIP]);
    if (!write_binary("wasm_pc.img", reinterpret_cast<uint8_t*>(&pc), sizeof(pc))) {
      spdlog::error("failed to checkpoint program counter");
    }
    spdlog::info("Checkpoint program counter");
    
    return pc;
}

void Checkpointer::checkpoint_memory() {
    // vector<uint8_t> memory = vm->get_memory().value_or(vector<uint8_t>(0));
    size_t memsize = vm->get_memsize().value_or(0);
    uintptr_t vmctx = regs[ENC_R14];
    // TODO: 0x50は固定じゃないので正しい値を取得できるようにする
    uintptr_t memptr = *(uintptr_t *)(vmctx+0x50);

    if (!write_binary("wasm_memory.img", reinterpret_cast<uint8_t*>(memptr), memsize)) {
      spdlog::error("failed to checkpoint memory");
    }
    spdlog::info("Checkpoint memory");
}

void Checkpointer::checkpoint_globals() {
    std::vector<global_t> global = vm->get_globals();
    struct globals g{global};
    vector<char> buffer = struct_pack::serialize(g);
    if (!write_binary("wasm_global.img", (uint8_t *)buffer.data(), buffer.size())) {
      spdlog::error("failed to checkpoint globals");
    }
    spdlog::info("Checkpoint globals");
}

bool Checkpointer::write_binary(const fs::path& filename, uint8_t *data, size_t size){
    std::filesystem::path full_path = dir / filename;

    std::ofstream fout(full_path, std::ios::out | std::ios::binary);
    fout.write((char *)data, size);
    fout.close();
    return true;
}
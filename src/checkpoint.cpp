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
  // get_memoryの返り値がnulloptの場合、checkpointせずにreturnする
    auto ret = vm->get_memory();
    if (!ret.has_value()) {
        spdlog::info("Uncheckpointed memory");
        return;
    }
    // get_memoryの返り値をMemory型にキャスト
    Memory mem = ret.value();
    if (!write_binary("wasm_memory.img", reinterpret_cast<uint8_t*>(mem.data), mem.size)) {
      spdlog::error("failed to checkpoint memory");
    }
    spdlog::info("Checkpoint memory");
}

void Checkpointer::checkpoint_globals() {
    Globals g = vm->get_globals();
    vector<char> buffer = struct_pack::serialize(g);
    if (!write_binary("wasm_globals.img", (uint8_t *)buffer.data(), buffer.size())) {
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
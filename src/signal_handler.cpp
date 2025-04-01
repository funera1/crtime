#include "signal_handler.h"
#include "regs.h"
#include "stack.h"
#include "utils.h"
#include "vmcxt.h"
#include <spdlog/spdlog.h>
#include <ucontext.h>
#include <fmt/ranges.h>
#include <ylt/struct_pack.hpp>

static char altstack[8192];
static VMCxt *global_vm;

void register_sigtrap(VMCxt *vm, SignalHandler handler, GlobalVmSetter setter) {
    setter(vm);

    struct sigaction sa {};
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sa.sa_sigaction = handler;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGTRAP, &sa, nullptr) == -1) {
        spdlog::error("Error: cannot handle SIGTRAP");
        exit(-1);
    }

    stack_t ss;
    ss.ss_sp = altstack;
    ss.ss_size = sizeof(altstack);
    ss.ss_flags = 0;
    sigaltstack(&ss, NULL);
}

void global_vm_setter(VMCxt *vm) {
    global_vm = vm;
}

void sigtrap_handler(int sig, siginfo_t *info, void *context) {
    /* printf("Caught SIGTRAP (signal number: %d)\n", sig); */
    spdlog::info("Caught SIGTRAP");

    // 最初にレジスタ全部退避させておく
    ucontext_t *ctx = (ucontext_t *)context;
    std::vector<uintptr_t> regs = save_regs(ctx);
    spdlog::info("Save registers");

    // checkpoint the program counter
    auto ret = global_vm->get_address_map();
    if (!ret.has_value()) {
        spdlog::error("failed to get address map");
        exit(1);
    }
    AddressMap addrmap = ret.value();
    spdlog::info("Get address map");

    // checkpoint program counter
    uint32_t pc = addrmap.get_wasm_offset(regs[ENC_RIP]);
    if (!write_binary("wasm_pc.img", reinterpret_cast<uint8_t*>(&pc), sizeof(pc))) {
      spdlog::error("failed to checkpoint program counter");
    }
    spdlog::info("Checkpoint program counter");

    // checkpoint stack
    vector<wasmtime_ssmap_entry_t> stack_size_maps = global_vm->get_stack_size_maps();
    spdlog::info("Get stack size map");
    vector<int> stack = reconstruct_stack(regs, stack_size_maps, pc);
    spdlog::debug("stack: [{}]", fmt::join(stack, ", "));
    spdlog::info("Reconstruct stack");

    if (!write_binary("wasm_stack.img", reinterpret_cast<uint8_t*>(&stack), sizeof(int) * stack.size())) {
      spdlog::error("failed to checkpoint stack");
    }
    spdlog::info("Checkpoint stack");

    // checkpoint the memory
    vector<uint8_t> memory = global_vm->get_memory().value_or(vector<uint8_t>(0));
    if (!write_binary("wasm_memory.img", memory.data(), memory.size())) {
      spdlog::error("failed to checkpoint memory");
    }
    spdlog::info("Checkpoint memory");

    // checkpoint globals
    std::vector<global_t> global = global_vm->get_globals();
    struct globals g{global};
    vector<char> buffer = struct_pack::serialize(g);
    if (!write_binary("wasm_global.img", (uint8_t *)buffer.data(), buffer.size())) {
      spdlog::error("failed to checkpoint globals");
    }
    spdlog::info("Checkpoint globals");

    // resume registers
    resume_regs(ctx, regs);
    spdlog::info("Resume registers");
}
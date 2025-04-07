#include "signal_handler.h"
#include <spdlog/spdlog.h>
#include <ucontext.h>
#include <fmt/ranges.h>
#include <ylt/struct_pack.hpp>

#include <checkpoint.h>
#include "regs.h"
#include "stack.h"
#include "utils.h"

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
    spdlog::info("Caught SIGTRAP");

    // 最初にレジスタ全部退避させておく
    ucontext_t *ctx = (ucontext_t *)context;
    std::vector<uintptr_t> regs = save_regs(ctx);
    spdlog::info("Save registers");
    
    Checkpointer C(global_vm, regs);

    // checkpoint module
    uint32_t pc = C.checkpoint_pc();
    C.checkpoint_stack(pc);
    C.checkpoint_locals(pc);
    C.checkpoint_memory();
    C.checkpoint_globals();

    // resume registers
    resume_regs(ctx, regs);
    spdlog::info("Resume registers");
}
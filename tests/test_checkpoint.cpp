#include <gtest/gtest.h>
#include "option.h"
#include "vmcxt.h"
#include "signal_handler.h"
#include "regs.h"
#include "stack.h"

vector<int> test_stack;
static VMCxt *global_test_vm;

void set_global_test_vm(VMCxt *vm) {
    global_test_vm = vm;
}

void test_handler(int sig, siginfo_t *info, void *context) {
    // 最初にレジスタ全部退避させておく
    ucontext_t *ctx = (ucontext_t *)context;
    std::vector<uintptr_t> regs = save_regs(ctx);

    // checkpoint the program counter
    auto ret = global_test_vm->get_address_map();
    if (!ret.has_value()) {
    }
    AddressMap addrmap = ret.value();

    // checkpoint program counter
    uint32_t pc = addrmap.get_wasm_offset(regs[ENC_RIP]);
    // checkpoint stack
    vector<wasmtime_ssmap_entry_t> stack_size_maps = global_test_vm->get_stack_size_maps();
    test_stack = reconstruct_stack(regs, stack_size_maps, pc);
}


void exec_local() {
    std::string wat = R"(
(module
  (func $start (export "_start")
    (local $a i32)
    (local $b i32)

    ;; ローカル変数に値を設定
    i32.const 5
    local.set $a       ;; $a = 5

    i32.const 10
    local.set $b       ;; $b = 10

    local.get $a       ;; スタックに$aの値（5）を積む
    local.get $b       ;; スタックに$bの値（10）を積む
    i32.add            ;; 5 + 10 = 15
    nop                ;; チェックポイント発生. スタックが15ならOK
    drop
  )
)
)";
    Option option(wat, false, false);
    VMCxt vm(option);
    if (!vm.initialize()) {
        // error
        spdlog::error("failed vmcxt init");
    }

    register_sigtrap(&vm, test_handler, set_global_test_vm);
    
    if (!vm.execute()) {

    }
}

TEST(TestCase, exec_local) {
    vector<int> expect{15};
    exec_local();
    EXPECT_EQ(expect, test_stack);
}
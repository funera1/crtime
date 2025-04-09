#include <gtest/gtest.h>
#include "option.h"
#include "vmcxt.h"
#include "signal_handler.h"
#include "regs.h"
#include <checkpoint.h>
#include "stack.h"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
fs::path test_dir;

class TempFileTest : public ::testing::Test {
    void SetUp() override {
        test_dir = fs::temp_directory_path() / "gtest_tmp_dir";
        fs::create_directories(test_dir);
    }

    void TearDown() override {
        fs::remove_all(test_dir);  // テスト終了後に削除
    }
};

static VMCxt *global_test_vm;
void global_test_vm_setter(VMCxt *vm) {
    global_test_vm = vm;
}

void test_restore_handler(int sig, siginfo_t *info, void *context) {
    spdlog::info("test_handler");

    // 最初にレジスタ全部退避させておく
    ucontext_t *ctx = (ucontext_t *)context;
    std::vector<uintptr_t> regs = save_regs(ctx);
    spdlog::info("save registers");
    
    Checkpointer C(global_test_vm, regs, test_dir);
    spdlog::info("checkpointer new");

    uint32_t pc = C.checkpoint_pc();
    C.checkpoint_stack(pc);
    C.checkpoint_locals(pc);
    C.checkpoint_memory();
    C.checkpoint_globals();

    // resume registers
    resume_regs(ctx, regs);
}

using wasmtime_vec = vector<wasmtime_val_t>;

// TODO: test_checkpointのexec_watと統合する
optional<wasmtime_vec> exec_wat_2(std::string wat) {
    Option option(wat, false, false);
    VMCxt vm(option);
    if (!vm.initialize()) {
        // error
        spdlog::error("failed vmcxt init");
    }

    register_sigtrap(&vm, test_restore_handler, global_test_vm_setter);
    
    return vm.execute();
}

optional<wasmtime_vec> restore_wat(std::string wat) {
    Option option(wat, false, false);
    RestoreOption ropt(true, test_dir);
    option.set_restore_opt(ropt);

    VMCxt vm(option);
    if (!vm.initialize()) {
        spdlog::error("failed vmcxt init");
    }

    return vm.execute();
}

void assert_eq_wasmtime_vec(wasmtime_vec expect, wasmtime_vec actual) {
    ASSERT_EQ(expect.size(), actual.size());

    size_t size = expect.size();
    for (int i = 0; i < size; i++) {
        ASSERT_EQ(expect[i].kind, actual[i].kind);

        switch (expect[i].kind) {
            case WASM_I32:
                ASSERT_EQ(expect[i].of.i32, actual[i].of.i32);
                break;
            case WASM_F32: 
                ASSERT_EQ(expect[i].of.f32, actual[i].of.f32);
                break;
            case WASM_I64:
                ASSERT_EQ(expect[i].of.i64, actual[i].of.i64);
                break;
            case WASM_F64:
                ASSERT_EQ(expect[i].of.f64, actual[i].of.f64);
                break;
            default:
                spdlog::error("Not supported type");
                ASSERT_TRUE(false);
                break;
        }
    }
}

TEST_F(TempFileTest, restore_add) {
    std::string wat = R"(
(module
  (func $start (export "_start") (result i32)
    ;; ローカル変数に値を設定
    i32.const 5
    i32.const 10
    i32.add

    nop                ;; checkpoint
  )
)
)";
    spdlog::info("dir: {}", test_dir.string());

    wasmtime_vec expect, actual;
    expect = exec_wat_2(wat).value();
    actual = restore_wat(wat).value();

    assert_eq_wasmtime_vec(expect, actual);
}

TEST_F(TempFileTest, restore_local) {
    std::string wat = R"(
(module
  (func $start (export "_start") (result i32)
    (local $a i32)
    (local $b i32)

    ;; ローカル変数に値を設定
    i32.const 5
    local.set $a       ;; $a = 5

    i32.const 10
    local.set $b       ;; $b = 10

    nop                ;; checkpoint

    local.get $a       ;; スタックに$aの値（5）を積む
    local.get $b       ;; スタックに$bの値（10）を積む
    i32.add            ;; 5 + 10 = 15
  )
)
)";
    wasmtime_vec expect, actual;
    expect = exec_wat_2(wat).value();
    actual = restore_wat(wat).value();
    assert_eq_wasmtime_vec(expect, actual);
}

TEST_F(TempFileTest, restore_memory) {
    std::string wat = R"(
(module
  (memory $mem 1) ;; メモリサイズ 1ページ（64KB）を確保
  (export "memory" (memory $mem))

  (func $start (export "_start") (result i32)
    ;; メモリにデータを書き込む
    i32.const 0      ;; メモリのオフセット 0
    i32.const 42     ;; 書き込む値
    i32.store        ;; メモリオフセット0に42を書き込む

    nop              ;; checkpoint

    ;; メモリからデータを読み込む
    i32.const 0      ;; メモリのオフセット 0
    i32.load         ;; メモリから値を読み込む（42が読み込まれる）
  )
)
)";
    wasmtime_vec expect, actual;
    expect = exec_wat_2(wat).value();
    actual = restore_wat(wat).value();
    assert_eq_wasmtime_vec(expect, actual);
}

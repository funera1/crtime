#include <gtest/gtest.h>
#include "option.h"
#include "vmcxt.h"
#include "signal_handler.h"
#include "regs.h"
#include "stack.h"

Stack test_stack;
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

void exec_wat(std::string wat) {
    Option option(wat, false, false);
    VMCxt vm(option);
    if (!vm.initialize()) {
        // error
        spdlog::error("failed vmcxt init");
    }

    register_sigtrap(&vm, test_handler, set_global_test_vm);
    
    if (!vm.execute()) {
        spdlog::error("failed vmcxt execute");
    }
}


TEST(TestCase, exec_local) {
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
    vector<uint32_t> expect{15};
    exec_wat(wat);
    EXPECT_EQ(expect, get_stack_vals(test_stack));
}

TEST(TestCase, exec_memory) {
    std::string wat = R"(
(module
  (memory $mem 1) ;; メモリサイズ 1ページ（64KB）を確保

  (func $start (export "_start")
    ;; メモリにデータを書き込む
    i32.const 0      ;; メモリのオフセット 0
    i32.const 42     ;; 書き込む値
    i32.store        ;; メモリオフセット0に42を書き込む

    ;; メモリからデータを読み込む
    i32.const 0      ;; メモリのオフセット 0
    i32.load         ;; メモリから値を読み込む（42が読み込まれる）
    nop              ;; スタックには[42]が積まれている

    drop             ;; スタックから結果を削除（42がスタックに残る）
  )
)

)";
    vector<uint32_t> expect{42};
    exec_wat(wat);
    EXPECT_EQ(expect, get_stack_vals(test_stack));
}

TEST(TestCase, exec_add_1) {
    std::string wat = R"(
(module
  (func $start (export "_start")
    i32.const 1
    i32.const 2
    i32.const 3
    i32.add         ;; [5]
    i32.add         ;; [6]
    nop
    drop
  )
)
)";
    vector<uint32_t> expect{6};
    exec_wat(wat);
    EXPECT_EQ(expect, get_stack_vals(test_stack));
}

TEST(TestCase, exec_add_2) {
    std::string wat = R"(
(module
  (func $start (export "_start")
    i32.const 1
    i32.const 2
    i32.const 3
    i32.add         ;; [5]
    nop
    i32.add         ;; [6]
    drop
  )
)
)";
    vector<uint32_t> expect{5};
    exec_wat(wat);
    EXPECT_EQ(expect, get_stack_vals(test_stack));
}

TEST(TestCase, exec_drop) {
    std::string wat = R"(
(module
  (func $start (export "_start")
    i32.const 1
    i32.const 2
    i32.const 3     ;; [1, 2, 3]
    drop            ;; [1, 2]
    i32.add         ;; [3]
    i32.const 4     ;; [3, 4]
    i32.add         ;; [7]
    nop             ;; [7]
    drop
  )
)
)";
    vector<uint32_t> expect{7};
    exec_wat(wat);
    EXPECT_EQ(expect, get_stack_vals(test_stack));
}

TEST(TestCase, exec_br) {
    std::string wat = R"(
(module
  (func $start (export "_start")
    (block $my_block
      i32.const 1
      i32.const 2
      br 0
      drop ;; スタックの値を破棄
    )
    i32.const 3
    i32.const 4
    i32.add
    nop             ;; [7]
    drop
  )
)
)";
    vector<uint32_t> expect{7};
    exec_wat(wat);
    EXPECT_EQ(expect, get_stack_vals(test_stack));
}

TEST(TestCase, exec_loop) {
    std::string wat = R"(
(module
  (func $start (export "_start")
    (local $i i32)  ;; ループカウンタ
    (local $sum i32)  ;; 合計値

    i32.const 0
    local.set $i  ;; $i = 0

    i32.const 0
    local.set $sum  ;; $sum = 0

    (block $exit
      (loop $loop
        local.get $i
        i32.const 10
        i32.ge_s  ;; $i >= 10 ならループ終了
        if 
          br $exit  ;; ループを抜ける
        end

        ;; sum += i
        local.get $sum
        local.get $i
        i32.add
        nop                 ;; 最後は1-9を足した[45]になる
        local.set $sum

        ;; i++
        local.get $i
        i32.const 1
        i32.add
        local.set $i
        br $loop  ;; ループの先頭に戻る
      )
    )
  )
)
)";
    vector<uint32_t> expect{45};
    exec_wat(wat);
    EXPECT_EQ(expect, get_stack_vals(test_stack));
}

// 返り値を返すblockの実行
TEST(TestCase, exec_return_val_block) {
    std::string wat = R"(
(module
  ;; loop を含む block のテスト
  ;; 10 回ループして 0 + 1 + 2 + ... + 9 の合計 45 を返す
  (func $start (export "_start")
    (local $i i32)  ;; カウンタ
    (local $sum i32)  ;; 合計値
    (local.set $i (i32.const 0))
    (local.set $sum (i32.const 0))

    (block $exit (result i32)
      (loop $loop
        local.get $i
        i32.const 10
        i32.ge_s
        if
          local.get $sum  ;; ブロックの返り値として sum を積む
          br $exit
        end

        local.get $sum
        local.get $i
        i32.add
        local.set $sum

        local.get $i
        i32.const 1
        i32.add
        local.set $i

        br $loop
      )
      i32.const 0  ;; 到達しないがブロックの構文上必要
    )
    nop
    drop
  )
)
)";
    vector<uint32_t> expect{45};
    exec_wat(wat);
    EXPECT_EQ(expect, get_stack_vals(test_stack));
}
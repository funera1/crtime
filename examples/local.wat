(module
  (func $start (export "_start")
    ;; ローカル変数を宣言
    (local $a i32)
    (local $b i32)

    ;; ローカル変数に値を設定
    i32.const 5
    local.set $a       ;; $a = 5
    nop

    i32.const 10
    local.set $b       ;; $b = 10
    nop

    ;; ローカル変数を加算して結果をスタックに積む
    local.get $a       ;; スタックに$aの値（5）を積む
    local.get $b       ;; スタックに$bの値（10）を積む
    i32.add            ;; 5 + 10 = 15
    nop
    drop               ;; スタックから結果を削除（計算結果を捨てる）
  )
)

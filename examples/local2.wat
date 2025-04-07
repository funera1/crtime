(module
  (func $start (export "_start")
    ;; ローカル変数を宣言
    (local $a i32)
    (local $b i64)

    ;; ローカル変数に値を設定
    i32.const 5
    local.set $a       ;; $a = 5

    i64.const 10
    local.set $b       ;; $b = 10

    nop
  )
)

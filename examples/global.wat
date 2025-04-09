(module
  (global $a (export "a") (mut i32) (i32.const 77232))
  (global $b (export "b") (mut i32) (i32.const 0))

  (func $start (export "_start") (result i32)
    ;; ローカル変数に値を設定
    i32.const 5
    global.set $a       ;; $a = 5

    i32.const 10
    global.set $b       ;; $b = 10

    nop

    ;; ローカル変数を加算して結果をスタックに積む
    global.get $a       ;; スタックに$aの値（5）を積む
    global.get $b       ;; スタックに$bの値（10）を積む
    i32.add            ;; 5 + 10 = 15
  )
)

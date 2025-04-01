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

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
        nop
        if 
          br $exit  ;; ループを抜ける
        end

        ;; sum += i
        local.get $sum
        local.get $i
        i32.add
        local.set $sum

        ;; i++
        local.get $i
        i32.const 1
        i32.add
        nop
        local.set $i

        br $loop  ;; ループの先頭に戻る
      )
    )
    nop
  )
)

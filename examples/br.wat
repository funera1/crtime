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
    nop
    drop
  )
)

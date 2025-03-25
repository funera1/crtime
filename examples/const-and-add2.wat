(module
  (func $start (export "_start")
    i32.const 1
    i32.const 2
    i32.add
    nop
    i32.const 3
    nop
    i32.add
    nop
    drop
  )
)

(module
  (func $start (export "_start")
    i32.const 1
    i32.const 2
    i32.add
    nop
    drop
  )

  (func $add
    i32.const 3
    i32.const 4
    i32.add
    drop
  )
)

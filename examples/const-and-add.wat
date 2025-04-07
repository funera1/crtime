(module
  (func $start (export "_start") (result i32)
    (local i32)
    i32.const 1
    i32.const 2
    i32.add
    nop
    i32.const 3
    i32.add
  )
)

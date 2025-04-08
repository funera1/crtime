(module
  (memory $mem 1) ;; メモリサイズ 1ページ（64KB）を確保
  (export "memory" (memory $mem))
  (data $.rodata (i32.const 16) "123456789ABCDEF")

  (func $start (export "_start") (result i32)
    ;; メモリにデータを書き込む
    i32.const 0      ;; メモリのオフセット 0
    i32.const 42     ;; 書き込む値
    i32.store        ;; メモリオフセット0に42を書き込む

    nop

    ;; メモリからデータを読み込む
    i32.const 0      ;; メモリのオフセット 0
    i32.load         ;; メモリから値を読み込む（42が読み込まれる）
  )
)

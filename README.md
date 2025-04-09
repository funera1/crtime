# crtime
The checkpoint/restore runtime using Wasmtime's Winch

## How to build and run
```bash
$ git submodule update --init --recursive
$ mkdir build && cd build
$ cmake ..
$ make -j
$ ./crtime -f ../examples/loop.wasm
$ ls
wasm_memory.img wasm_pc.img wasm_stack.img wasm_locals.img
$ ./crtime -r -f ../examples/loop.wasm
```

## Command help
```
Run WebAssembly modules with Wasmtime
Usage:
  WASM Executor [OPTION...]

  -f, --file arg       WASM file (required)
      --print-addrmap  Print address map
      --print-ssmap    Print stack size map
  -r, --restore        Restore mode
  -e, --explore        Explore mode
  -h, --help           Show help
```

## TODO
- [x] C/R memory
- [ ] C/R global
- [x] C/R program counter
- [x] C/R stack
- [ ] 任意の実行ポイントでcheckpoint
- [ ] exportされていないmemory/globalのC/R
- [ ] 関数呼び出し対応

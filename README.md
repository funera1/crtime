# crtime
The checkpoint/restore runtime using Wasmtime's Winch

## How to build and run
```bash
$ git submodule update --init --recursive
$ mkdir build && cd build
$ cmake ..
$ make -j
$ ./crtime ../examples/binary-trees.wasm
$ ls
wasm_memory.img wasm_global.img
```

## TODO
- [x] checkpoint memory
- [x] checkpoint global
- [ ] checkpoint program counter
- [ ] checkpoint stack
- [ ] restore memory
- [ ] restore global
- [ ] restore program counter
- [ ] restore stack
- [ ] 任意の実行ポイントでcheckpoint

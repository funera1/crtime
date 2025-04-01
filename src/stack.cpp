#include <iostream>
#include <fstream>
#include <string>
#include <cassert>

#include <stdio.h>
#include <stdlib.h>
#include <format>

#include <spdlog/spdlog.h>
#include <wasmtime.h>
#include <wasm.h>
#include "regs.h"
#include "stack.h"

void check_magic_number(uintptr_t rbp) {
    uint32_t magic = *(uint32_t *)(rbp-4);
    if (magic != 0xdeadbeaf) {
      spdlog::error("Not match magic number");
    }
    spdlog::info("Check magic number\n");
}

vector<int> reconstruct_stack(vector<uintptr_t> &regs, vector<wasmtime_ssmap_entry_t> &stack_size_map, uint32_t pc) {
  uintptr_t rbp = regs[ENC_RBP];
  check_magic_number(rbp);

  // stack_sizeを取得
  spdlog::debug("stack_size_map size: {:d}", stack_size_map.size());
  uint32_t stack_size = -1;
  for (int i = 0; i < stack_size_map.size(); i++) {
    if (stack_size_map[i].wasm_offset == pc) {
      stack_size = stack_size_map[i].stack_size;
      break;
    }
  }

  if (stack_size == -1) {
    spdlog::error("Failed to get stack size");
    // TODO: 返り値を見直す
    return vector<int>(0);
  }
  spdlog::debug("stack size: {:d}", stack_size);

  // metadataを取得
  vector<uint32_t> v(0); 
  for (int i = 1; i < stack_size+1; i++) {
    uint32_t metadata = *(uint32_t *)(rbp-4*(i+1));
    assert(0 <= metadata);
    v.push_back(metadata);
  }

  // metadataからスタックの値を取得
  vector<int> stack(stack_size);
  for (int i = 0; i < v.size(); i++) {
    // vにはreg.hw_encかmemoryのoffsetが入っている
    // 16未満ならreg, 16以上ならmemory
    if (v[i] < 16) {
      stack[i] = regs[v[i]];
    }
    else {
        // TODO
    }
  }
  
  return stack;
}
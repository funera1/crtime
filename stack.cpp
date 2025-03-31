#include <iostream>
#include <fstream>
#include <string>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <format>

#include <spdlog/spdlog.h>
#include <wasmtime.h>
#include <wasm.h>
#include "regs.h"
#include "stack.h"

void check_magic_number(uintptr_t rsp) {
    uint32_t magic = *(uint32_t *)(rsp + 100);
    assert(magic == 0xdeadbeaf);
    spdlog::debug("Check magic number\n");
}

vector<int> reconstruct_stack(vector<uintptr_t> &regs, vector<wasmtime_stacksizemap_entry_t> &stack_size_map, uint32_t pc) {
  uintptr_t rsp = regs[ENC_RSP];
  check_magic_number(rsp);

  // TODO: stack_sizeの正しい値を取得
  uint32_t stack_size = 1;
  for (int i = 0; i < stack_size_map.size(); i++) {
    if (stack_size_map[i].wasm_offset == pc) {
      stack_size = stack_size_map[i].stack_size;
    }
  }
  assert(stack_size != -1);
  spdlog::debug("stack size: {:d}", stack_size);

  // metadataを取得
  vector<int> v(0); 
  for (int i = 1; i < stack_size+1; i++) {
    uint32_t metadata = *(uint32_t *)(rsp + 200 + i);
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

void print_stack(vector<uintptr_t> regs) {
  uintptr_t rsp = regs[ENC_RSP];
  check_magic_number(rsp);

  // TODO: stack_sizeの正しい値を取得
  const int stack_size = 1;

  // metadataを取得
  vector<int> v(0); 
  for (int i = 1; i < stack_size+1; i++) {
    uint32_t metadata = *(uint32_t *)(rsp + 200 + i);
    v.push_back(metadata);
  }
  
  string s = "reg_ids: [";
  for (auto vi : v) s += std::format("{} ", to_string(vi));
  s.push_back(']');
  spdlog::debug(s);
  
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
  
  // print stack
  s = "stack_contents: [";
  for (auto si : stack) s += std::format("{} ", to_string(si));
  s.push_back(']');
  spdlog::debug(s);
}
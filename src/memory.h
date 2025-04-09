#pragma once
#include <stdint.h>
#include <string>
#include <vector>
#include <optional>
#include <signal.h>
#include <wasmtime.h>

const size_t WASM_PAGE_SIZE = 65536;

struct Memory {
private:
  wasmtime_memory_t *memory;
public:
  uint8_t *data;
  size_t size;
  
  Memory() = default;
  Memory(uint8_t *d, size_t s) : memory(NULL), data(d), size(s) {}
  Memory(wasmtime_memory_t *m, uint8_t *d, size_t s) : memory(m), data(d), size(s) {}
  
  bool apply_memory();
};

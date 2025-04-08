#pragma once
#include <stdint.h>
#include <string>
#include <vector>
#include <optional>
#include <signal.h>
#include <wasmtime.h>

struct Memory {
  wasmtime_memory_t memory;
  uint8_t *data;
  size_t size;
  
  Memory() = default;
  Memory(wasmtime_memory_t m, uint8_t *d, size_t s) : memory(m), data(d), size(s) {}
  
  bool apply_memory();
};

#pragma once
#include <wasmtime.h>
#include <string>

void exit_with_error(std::string message, wasmtime_error_t *error, wasm_trap_t *trap);
wasm_byte_vec_t load_wasm_file(std::string file_name);
void instantiate_wasi(wasmtime_context_t *context, wasm_trap_t *trap);
bool write_binary(std::string filepath, uint8_t *data, size_t size);
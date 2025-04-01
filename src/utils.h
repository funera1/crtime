#pragma once
#include <wasmtime.h>

void exit_with_error(std::string message, wasmtime_error_t *error, wasm_trap_t *trap);
wasm_byte_vec_t load_wasm_file(std::string file_name);
void instantiate_wasi(wasmtime_context_t *context, wasm_trap_t *trap);
#pragma once
#include <wasmtime.h>
#include <string>
#include <vector>

void exit_with_error(std::string message, wasmtime_error_t *error, wasm_trap_t *trap);
void instantiate_wasi(wasmtime_context_t *context, wasm_trap_t *trap);
bool write_binary(std::string filepath, uint8_t *data, size_t size);

wasm_byte_vec_t load_wasm_from_file(std::string file_name);
wasm_byte_vec_t load_wasm_form_wasm(std::vector<uint8_t> code);
wasm_byte_vec_t load_wasm_from_wat(std::string wat);
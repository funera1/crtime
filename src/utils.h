#pragma once

#include <wasmtime.h>
static void exit_with_error(const char *message, wasmtime_error_t *error, wasm_trap_t *trap);
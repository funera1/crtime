#pragma once
#include <string>
#include <stdint.h>
#include <wasmtime.h>
#include "stack.h"
#include "option.h"

uint32_t parse_pc(std::string dir);
Stack parse_stack(std::string dir);
Memory parse_memory(string dir);
Globals parse_globals(string dir);

void set_restore_info(wasm_config_t* config, RestoreOption opt);
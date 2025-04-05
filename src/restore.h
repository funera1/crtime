#pragma once
#include <string>
#include <stdint.h>
#include "stack.h"

uint32_t parse_pc(std::string dir);
Stack parse_stack(std::string dir);
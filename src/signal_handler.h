#pragma once
#include <signal.h>
#include "vmcxt.h"

void register_sigtrap(VMCxt *vm);
void sigtrap_handler(int sig, siginfo_t *info, void *context);
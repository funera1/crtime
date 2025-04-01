#pragma once
#include <signal.h>
#include "vmcxt.h"

using SignalHandler = void (*)(int , siginfo_t *, void *);
using GlobalVmSetter = void (*)(VMCxt *);

void register_sigtrap(VMCxt *vm, SignalHandler handler, GlobalVmSetter setter);
void sigtrap_handler(int sig, siginfo_t *info, void *context);
void global_vm_setter(VMCxt* vm);
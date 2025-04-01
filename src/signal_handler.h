#pragma once
#include <signal.h>

void register_sigtrap();
void sigtrap_handler(int sig, siginfo_t *info, void *context);
#include "signal_handler.h"
#include <spdlog/spdlog.h>
#include <ucontext.h>

static char altstack[8192];

void register_sigtrap() {
    struct sigaction sa {};
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sa.sa_sigaction = sigtrap_handler;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGTRAP, &sa, nullptr) == -1) {
        spdlog::error("Error: cannot handle SIGTRAP");
        exit(-1);
    }

    stack_t ss;
    ss.ss_sp = altstack;
    ss.ss_size = sizeof(altstack);
    ss.ss_flags = 0;
    sigaltstack(&ss, NULL);
}

void sigtrap_handler(int sig, siginfo_t *info, void *context) {
    spdlog::info("Caught SIGTRAP");
}
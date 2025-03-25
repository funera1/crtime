#include <stdint.h>
#include <ucontext.h>

#include <vector>
#include <string>
#include "regs.h"

using namespace std;

// const uint8_t ENC_RAX = 0;
// const uint8_t ENC_RCX = 1;
// const uint8_t ENC_RDX = 2;
// const uint8_t ENC_RBX = 3;
// const uint8_t ENC_RSP = 4;
// const uint8_t ENC_RBP = 5;
// const uint8_t ENC_RSI = 6;
// const uint8_t ENC_RDI = 7;
// const uint8_t ENC_R8 = 8;
// const uint8_t ENC_R9 = 9;
// const uint8_t ENC_R10 = 10;
// const uint8_t ENC_R11 = 11;
// const uint8_t ENC_R12 = 12;
// const uint8_t ENC_R13 = 13;
// const uint8_t ENC_R14 = 14;
// const uint8_t ENC_R15 = 15;

inline string realreg_name(uint32_t reg_hw_enc) {
    switch (reg_hw_enc) {
      case ENC_RAX:
        return "%rax";
      default:
        return "hoge";
    }
}

vector<uintptr_t> save_regs(ucontext_t *ctx) {
  vector<uintptr_t> regs(16); 
  regs[ENC_RAX] = ctx->uc_mcontext.gregs[REG_RAX];
  regs[ENC_RCX] = ctx->uc_mcontext.gregs[REG_RCX];
  regs[ENC_RDX] = ctx->uc_mcontext.gregs[REG_RDX];
  regs[ENC_RBX] = ctx->uc_mcontext.gregs[REG_RBX];
  regs[ENC_RSP] = ctx->uc_mcontext.gregs[REG_RSP];
  regs[ENC_RBP] = ctx->uc_mcontext.gregs[REG_RBP];
  regs[ENC_RSI] = ctx->uc_mcontext.gregs[REG_RSI];
  regs[ENC_RDI] = ctx->uc_mcontext.gregs[REG_RDI];
  regs[ENC_R8] = ctx->uc_mcontext.gregs[REG_R8];
  regs[ENC_R9] = ctx->uc_mcontext.gregs[REG_R9];
  regs[ENC_R10] = ctx->uc_mcontext.gregs[REG_R10];
  regs[ENC_R11] = ctx->uc_mcontext.gregs[REG_R11];
  regs[ENC_R12] = ctx->uc_mcontext.gregs[REG_R12];
  regs[ENC_R13] = ctx->uc_mcontext.gregs[REG_R13];
  regs[ENC_R14] = ctx->uc_mcontext.gregs[REG_R14];
  regs[ENC_R15] = ctx->uc_mcontext.gregs[REG_R15];
  
  return regs;
}
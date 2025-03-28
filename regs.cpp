#include <stdint.h>
#include <ucontext.h>

#include <vector>
#include <string>
#include "regs.h"

using namespace std;

inline string realreg_name(uint32_t reg_hw_enc) {
    switch (reg_hw_enc) {
      case ENC_RAX:
        return "%rax";
      default:
        return "hoge";
    }
}

vector<uintptr_t> save_regs(ucontext_t *ctx) {
  vector<uintptr_t> regs(17); 
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
  regs[ENC_RIP] = ctx->uc_mcontext.gregs[REG_RIP];
  
  return regs;
}

void resume_regs(ucontext_t *ctx, vector<uintptr_t> regs) {
  ctx->uc_mcontext.gregs[REG_RAX] = regs[ENC_RAX];
  ctx->uc_mcontext.gregs[REG_RCX] = regs[ENC_RCX];
  ctx->uc_mcontext.gregs[REG_RDX] = regs[ENC_RDX];
  ctx->uc_mcontext.gregs[REG_RBX] = regs[ENC_RBX];
  ctx->uc_mcontext.gregs[REG_RSP] = regs[ENC_RSP];
  ctx->uc_mcontext.gregs[REG_RBP] = regs[ENC_RBP];
  ctx->uc_mcontext.gregs[REG_RSI] = regs[ENC_RSI];
  ctx->uc_mcontext.gregs[REG_RDI] = regs[ENC_RDI];
  ctx->uc_mcontext.gregs[REG_R8] = regs[ENC_R8];
  ctx->uc_mcontext.gregs[REG_R9] = regs[ENC_R9];
  ctx->uc_mcontext.gregs[REG_R10] = regs[ENC_R10];
  ctx->uc_mcontext.gregs[REG_R11] = regs[ENC_R11];
  ctx->uc_mcontext.gregs[REG_R12] = regs[ENC_R12];
  ctx->uc_mcontext.gregs[REG_R13] = regs[ENC_R13];
  ctx->uc_mcontext.gregs[REG_R14] = regs[ENC_R14];
  ctx->uc_mcontext.gregs[REG_R15] = regs[ENC_R15];
  ctx->uc_mcontext.gregs[REG_RIP] = regs[ENC_RIP];
}

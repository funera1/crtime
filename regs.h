#ifndef REGS_H
#define REGS_H

#include <stdint.h>
#include <ucontext.h>

#include <vector>
#include <string>

using namespace std;

const uint8_t ENC_RAX = 0;
const uint8_t ENC_RCX = 1;
const uint8_t ENC_RDX = 2;
const uint8_t ENC_RBX = 3;
const uint8_t ENC_RSP = 4;
const uint8_t ENC_RBP = 5;
const uint8_t ENC_RSI = 6;
const uint8_t ENC_RDI = 7;
const uint8_t ENC_R8 = 8;
const uint8_t ENC_R9 = 9;
const uint8_t ENC_R10 = 10;
const uint8_t ENC_R11 = 11;
const uint8_t ENC_R12 = 12;
const uint8_t ENC_R13 = 13;
const uint8_t ENC_R14 = 14;
const uint8_t ENC_R15 = 15;

vector<uintptr_t> save_regs(ucontext_t *ctx);

#endif
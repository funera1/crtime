#pragma once
#include <filesystem>
#include <fstream>
#include <vmcxt.h>
#include <stack.h>
#include <regs.h>

using namespace std;
namespace fs = std::filesystem;

class Checkpointer {
public:
    fs::path dir;
    VMCxt *vm;
    vector<uintptr_t> regs;
    
    Checkpointer() = default;
    ~Checkpointer() = default;

    Checkpointer(VMCxt *v, vector<uintptr_t> r): vm(v), regs(r), dir() {};
    Checkpointer(VMCxt *v, vector<uintptr_t> r, fs::path d): vm(v), regs(r), dir(d) {};

    void checkpoint_stack(uint32_t pc);
    void checkpoint_locals(uint32_t pc);
    uint32_t checkpoint_pc();
    void checkpoint_memory();
    void checkpoint_globals();
};
#pragma once
#include <string>
#include <wasm.h>
#include "utils.h"

class RestoreOption {
public:
    bool is_restore;
    std::string state_path;
    
    RestoreOption()
        : is_restore(false), state_path("") {}
    RestoreOption(bool is_restore)
        : is_restore(is_restore), state_path("") {}
    RestoreOption(bool is_restore, std::string path)
        : is_restore(is_restore), state_path(path) {}
};

class Option {
public:
    wasm_byte_vec_t wasm;
    bool is_print_addrmap;
    bool is_print_ssmap;
    RestoreOption restore_opt;

    // TODO: constructorが増えまくるので、値の入れ方を考える
    Option()
        : wasm(), is_print_addrmap(false), is_print_ssmap(false) {}
    Option(wasm_byte_vec_t wasm, bool addrmap, bool ssmap) 
        : wasm(wasm), is_print_addrmap(addrmap), is_print_ssmap(ssmap) {}
    Option(std::string wat, bool addrmap, bool ssmap) 
        : wasm(load_wasm_from_wat(wat)), is_print_addrmap(addrmap), is_print_ssmap(ssmap) {}

    // TODO: wasmが正しいものかを判定する
    bool is_valid() const { return true; }
};

Option parse_options(int argc, char* argv[]);
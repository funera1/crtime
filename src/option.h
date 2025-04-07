#pragma once
#include <string>
#include <wasm.h>
#include "utils.h"

class RestoreOption {
public:
    bool is_restore;
    std::string state_path;
    
    RestoreOption()
        : is_restore(false), state_path("./") {}
    RestoreOption(bool is_restore)
        : is_restore(is_restore), state_path("./") {}
    RestoreOption(bool is_restore, std::string path)
        : is_restore(is_restore), state_path(path + "/") {}
};

class Option {
public:
    std::string path;
    wasm_byte_vec_t wasm;
    bool is_print_addrmap;
    bool is_print_ssmap;
    RestoreOption restore_opt;
    bool is_explore;

    // TODO: constructorが増えまくるので、値の入れ方を考える
    Option()
        : path(), wasm(), is_print_addrmap(false), is_print_ssmap(false), is_explore(false) {}
    Option(wasm_byte_vec_t wasm, bool addrmap, bool ssmap) 
        : path(), wasm(wasm), is_print_addrmap(addrmap), is_print_ssmap(ssmap), is_explore(false) {}
    Option(std::string wat, bool addrmap, bool ssmap) 
        : path(), wasm(load_wasm_from_wat(wat)), is_print_addrmap(addrmap), is_print_ssmap(ssmap), is_explore(false) {}

    // TODO: wasmが正しいものかを判定する
    bool is_valid() const { return true; }
    void set_restore_opt(RestoreOption o);
};

Option parse_options(int argc, char* argv[]);
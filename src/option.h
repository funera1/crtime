#pragma once
#include <string>

class Option {
public:
    std::string file_name;
    bool is_print_addrmap;
    bool is_print_ssmap;

    Option()
        : file_name(""), is_print_addrmap(false), is_print_ssmap(false) {}
    Option(std::string file, bool addrmap, bool ssmap) 
        : file_name(file), is_print_addrmap(addrmap), is_print_ssmap(ssmap) {}

    bool is_valid() const { return !file_name.empty(); }
};

Option parse_options(int argc, char* argv[]);
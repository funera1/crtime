#include "option.h"
#include <cxxopts.hpp>
#include <iostream>

Option parse_options(int argc, char* argv[]) {
    cxxopts::Options options("WASM Executor", "Run WebAssembly modules with Wasmtime");
    options.add_options()
        ("f,file", "WASM file (required)", cxxopts::value<std::string>())
        ("print-addrmap", "Print address map", cxxopts::value<bool>()->default_value("false"))
        ("print-ssmap", "Print stack size map", cxxopts::value<bool>()->default_value("false"))
        ("r,restore", "Restore mode", cxxopts::value<bool>()->default_value("false"))
        ("h,help", "Show help");

    auto result = options.parse(argc, argv);

    if (result.count("help") || result.count("file") == 0) {
        std::cout << options.help() << std::endl;
        return Option();
    }
    
    Option opt = Option(
        load_wasm_from_file(result["file"].as<std::string>()),
        result["print-addrmap"].as<bool>(),
        result["print-ssmap"].as<bool>()
    );
    
    if (result["restore"].as<bool>()) {
        opt.restore_opt = RestoreOption(true);
    }
    
    return opt;
}
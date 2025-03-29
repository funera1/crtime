/*
Example of instantiating of the WebAssembly module and invoking its exported
function.

You can compile and run this example on Linux with:

   cargo build --release -p wasmtime-c-api
   cc examples/hello.c \
       -I crates/c-api/include \
       target/release/libwasmtime.a \
       -lpthread -ldl -lm \
       -o hello
   ./hello

Note that on Windows and macOS the command will be similar, but you'll need
to tweak the `-lpthread` and such annotations as well as the name of the
`libwasmtime.a` file on Windows.

You can also build using cmake:

mkdir build && cd build && cmake .. && cmake --build . --target wasmtime-hello
*/

#include <iostream>
#include <fstream>
#include <string>
#include <optional>
#include <format>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <ucontext.h>

#include <wasm.h>
#include <wasmtime.h>
#include <cxxopts.hpp>
#include <ylt/struct_pack.hpp>
#include <ylt/struct_json/json_writer.h>
#include <spdlog/spdlog.h>

#include "regs.h"
#include "stack.h"

using namespace std;

class VMCxt;
VMCxt *vm = nullptr;
#define ALTSTACK_SIZE 8192
static char altstack[ALTSTACK_SIZE];

static void exit_with_error(const char *message, wasmtime_error_t *error,
                            wasm_trap_t *trap);

typedef struct global {
  int kind;
  int64_t value;
} global_t;

struct globals {
  // vector<wasmtime_val_t> globals;
  vector<global_t> globals;
};

bool write_binary(string filepath, uint8_t *data, size_t size){
    std::ofstream fout(filepath, std::ios::out | std::ios::binary);
    fout.write((char *)data, size);
    fout.close();
    return true;
}

class AddressMap {
public:
    uintptr_t base_address;
    vector<wasmtime_addrmap_entry_t> addrmap;

    AddressMap(uintptr_t base_addr, vector<wasmtime_addrmap_entry_t> addrmap) : base_address(base_addr), addrmap(addrmap){}

    uint32_t get_wasm_offset(uintptr_t rip) {
        uint32_t pc_code_offset = rip - base_address;
        for (auto addr : addrmap) {
            if (addr.code_offset == pc_code_offset) {
              return addr.wasm_offset;
            }
        }
        spdlog::debug("Not found target wasm offset in address map");
        return 0xdeadbeaf;
    }
};
  
class VMCxt {
public:
  wasm_engine_t *engine;
  wasmtime_store_t *store;
  wasmtime_linker_t *linker;
  wasmtime_module_t *module; 
  wasmtime_context_t *context;
  wasm_trap_t *trap;

  VMCxt(wasm_config_t *config) {
    engine = wasm_engine_new_with_config(config);
    store = wasmtime_store_new(engine, NULL, NULL);
    linker = wasmtime_linker_new(engine);
    context = wasmtime_store_context(store);
    module = NULL;
    trap = NULL;
  }

  wasmtime_instance_t get_instance() {
    wasmtime_instance_t instance;
    wasmtime_error_t *error = wasmtime_linker_instantiate(linker, context, module, &instance, &trap);
    if (error != NULL || trap != NULL)
      exit_with_error("error instance module", error, trap);

    return instance;
  }
  
  optional<AddressMap> get_address_map() {
    // moduleのNULLチェック
    if (module == NULL) {
      spdlog::error("module is NULL");
      return nullopt;
    }

    // Wasmtimeからaddress_mapをもらう
    wasmtime_addrmap_entry_t* data;
    size_t len;
    uintptr_t base_addr;
    wasmtime_module_address_map(module, &data, &len, &base_addr);
    spdlog::debug("ptr: {}", static_cast<void *>(data));

    vector<wasmtime_addrmap_entry_t> address_map(data, data+len);
    
    string s = "(code offs, wasm offs): [";
    for (auto ad : address_map) s += format(" ({}, {}) ", ad.code_offset, ad.wasm_offset);
    s += "]";
    spdlog::debug("{:s}", s);
    
    return AddressMap(base_addr, address_map);
  }
  
  vector<vector<uint32_t>> get_stack_size_maps() {
    if (module == NULL) {
      spdlog::error("module is NULL");
    }
    
    // stack_size_mapsを取得
    uint32_t *ssmap_flatten;
    size_t *lens;
    size_t count = wasmtime_module_stack_size_maps(module, &ssmap_flatten, &lens);
    spdlog::info("return wasmtime_module_stack_size_maps");
    
    // **const u32をvec<vec<u32>>に変換
    vector<vector<uint32_t>> stack_size_maps(count);
    int pos = 0;
    for (size_t i = 0; i < count; i++) {
      stack_size_maps[i] = vector<uint32_t>(ssmap_flatten, ssmap_flatten+lens[i]);
      ssmap_flatten += lens[i];
      spdlog::debug("len: {:d}", lens[i]);
    }
    
    // debug
    string s = "(stack size maps): [";
    for (auto sm : stack_size_maps) {
      s += " [";
      for (int i = 0; i < sm.size(); i++) {
        s += format("{}", sm[i]);
        if (i != sm.size()-1) s += ", ";
      }
      s += "] ";
    }
    s += "]";
    spdlog::debug("{:s}", s);
    
    return stack_size_maps;
  }

  vector<uint8_t> get_memory() {
      wasmtime_instance_t instance = get_instance();
      wasmtime_extern_t export_;
      
      string name = "memory";
      bool ok = wasmtime_instance_export_get(context, &instance, name.c_str(), name.size(), &export_);
      if (!ok || export_.kind != WASMTIME_EXTERN_MEMORY) {
        spdlog::error("failed to get memory export");
        return vector<uint8_t>();
      }

      wasmtime_memory_t memory = export_.of.memory;
      uint8_t* data = wasmtime_memory_data(context, &memory);
      size_t size = wasmtime_memory_data_size(context, &memory);
      return vector<uint8_t>(data, data+size);
  }

  // TODO: 実装できてない, globalを事前に特定の名前でexportしないといけない設計になっていて良くない
  vector<global_t> get_globals() {
      wasmtime_instance_t instance = get_instance();
      wasmtime_extern_t export_;

      vector<global_t> globals;
      
      // TODO: 1文字しか対応できてない
      string base_name = "global_";
      for (int i = 0; i < 10; i++) {
        string name = base_name + to_string(i);
        bool ok = wasmtime_instance_export_get(context, &instance, name.c_str(), name.size(), &export_);
        // TODO: エラーとglobalが存在するかの判定ができるようにする
        if (!ok) {
          break;
        }

        wasmtime_global_t global = export_.of.global;
        wasmtime_val_t value;
        wasmtime_global_get(context, &global, &value);

        global_t g;
        g.kind = value.kind;
        switch(value.kind) {
          case WASMTIME_I32:
            g.value = value.of.i32;
            break;
          case WASMTIME_F32:
            g.value = value.of.f32;
            break;
          case WASMTIME_I64:
            g.value = value.of.i64;
            break;
          case WASMTIME_F64:
            g.value = value.of.f64;
            break;
          default:
            break;
        }
        globals.push_back(g);
      }

      return globals;
  }
};


// SIGTRAP シグナルハンドラ
void sigtrap_handler(int sig, siginfo_t *info, void *context) {
    /* printf("Caught SIGTRAP (signal number: %d)\n", sig); */
    spdlog::info("Caught SIGTRAP");

    // 最初にレジスタ全部退避させておく
    ucontext_t *ctx = (ucontext_t *)context;
    vector<uintptr_t> regs = save_regs(ctx);
    spdlog::info("Save registers");

    // checkpoint the program counter
    auto ret = vm->get_address_map();
    if (!ret.has_value()) {
        spdlog::error("failed to get address map");
        exit(1);
    }
    AddressMap addrmap = ret.value();
    spdlog::info("Get address map");

    // checkpoint stack
    vm->get_stack_size_maps();
    spdlog::info("Get stack size map");

    // PCのcode offsetからwasm offsetに変換し保存
    uint32_t pc = addrmap.get_wasm_offset(regs[ENC_RIP]);
    if (!write_binary("wasm_pc.img", reinterpret_cast<uint8_t*>(&pc), sizeof(pc))) {
      spdlog::error("failed to checkpoint program counter");
    }
    spdlog::info("Checkpoint program counter");

    // checkpoint the memory
    vector<uint8_t> memory = vm->get_memory();
    if (!write_binary("wasm_memory.img", memory.data(), memory.size())) {
      spdlog::error("failed to checkpoint memory");
    }
    spdlog::info("Checkpoint memory");

    // checkpoint globals
    vector<global_t> global = vm->get_globals();
    struct globals g{global};
    vector<char> buffer = struct_pack::serialize(g);
    if (!write_binary("wasm_global.img", (uint8_t *)buffer.data(), buffer.size())) {
      spdlog::error("failed to checkpoint globals");
    }
    spdlog::info("Checkpoint globals");

    // resume registers
    resume_regs(ctx, regs);
    spdlog::info("Resume registers");
}

void register_sigtrap() {
#if defined(_WIN32)
    signal(SIGILL, sigtrap_handler);
    // SPDLOG_DEBUG("SIGILL registered");
#else
    struct sigaction sa {};
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sa.sa_sigaction = sigtrap_handler;
    sigemptyset(&sa.sa_mask);

    // Register the signal handler for SIGTRAP
    if (sigaction(SIGTRAP, &sa, nullptr) == -1) {
        // SPDLOG_ERROR("Error: cannot handle SIGTRAP");
        exit(-1);
    }
    
    // 代替スタックの設定
    stack_t ss;
    ss.ss_sp = altstack;
    ss.ss_size = ALTSTACK_SIZE;
    ss.ss_flags = 0;
    sigaltstack(&ss, NULL);
#endif
}

void instantiate_wasi(wasmtime_context_t *context, wasm_trap_t *trap) {
  wasi_config_t *wasi_config = wasi_config_new();
  assert(wasi_config);
  wasi_config_inherit_argv(wasi_config);
  wasi_config_inherit_env(wasi_config);
  wasi_config_inherit_stdin(wasi_config);
  wasi_config_inherit_stdout(wasi_config);
  wasi_config_inherit_stderr(wasi_config);
  wasmtime_error_t *error = wasmtime_context_set_wasi(context, wasi_config);
  if (error != NULL)
    exit_with_error("failed to instantiate WASI", error, NULL);
}

wasm_byte_vec_t load_wasm_file(string file_name) {
  wasm_byte_vec_t wasm;

  FILE *file = fopen(file_name.c_str(), "rb");
  if (!file) {
    printf("> Error loading file!\n");
    exit(1);
  }
  fseek(file, 0L, SEEK_END);
  size_t file_size = ftell(file);
  wasm_byte_vec_new_uninitialized(&wasm, file_size);
  fseek(file, 0L, SEEK_SET);
  if (fread(wasm.data, file_size, 1, file) != 1) {
    printf("> Error loading module!\n");
    exit(1);
  }
  fclose(file);
  return wasm;
}

int main(int argc, char* argv[]) {
  // Init logger
  spdlog::set_level(spdlog::level::trace);
  spdlog::debug("Hello spdlog");

  // Parse options
  cxxopts::Options options("MyProgram", "One line description of MyProgram");
  options.add_options()
    ("f,file", "Please input wasm file (required)", cxxopts::value<std::string>())
    ("h,help", "help output", cxxopts::value<bool>()->default_value("false"))
    ;
  auto result = options.parse(argc, argv);

    // help オプションが指定されたらヘルプを表示して終了
    if (result.count("help") > 0 || result.count("file") == 0) {
        std::cout << options.help() << std::endl;
        return 0;
    }

  string file_name = result["file"].as<string>();
  assert(file_name.size() != 0);

  // Register sigtrap handler
  register_sigtrap();

  // Set up our context
  wasm_config_t* config = wasm_config_new();
  wasmtime_config_strategy_set(config, WASMTIME_STRATEGY_WINCH);

  vm = new VMCxt(config);
  assert(vm->engine != NULL);
  assert(vm->store != NULL);

  wasmtime_linker_allow_unknown_exports(vm->linker, true);

  // Create a linker with WASI functions defined
  wasmtime_error_t *error = wasmtime_linker_define_wasi(vm->linker);
  if (error != NULL)
    exit_with_error("failed to link wasi", error, NULL);

  // Load our input file to parse it next
  wasm_byte_vec_t wasm = load_wasm_file(file_name);

  // Compile our modules
  error = wasmtime_module_new(vm->engine, (uint8_t *)wasm.data, wasm.size, &vm->module);
  if (!vm->module)
    exit_with_error("failed to compile module", error, NULL);
  wasm_byte_vec_delete(&wasm);

  // Instantiate wasi
  instantiate_wasi(vm->context, vm->trap);

  // Instantiate the module
  error = wasmtime_linker_module(vm->linker, vm->context, "", 0, vm->module);
  if (error != NULL)
    exit_with_error("failed to instantiate module", error, NULL);

  // Run it.
  wasmtime_func_t func;
  error = wasmtime_linker_get_default(vm->linker, vm->context, "", 0, &func);
  if (error != NULL)
    exit_with_error("failed to locate default export for module", error, NULL);

  error = wasmtime_func_call(vm->context, &func, NULL, 0, NULL, 0, &vm->trap);
  if (error != NULL || vm->trap != NULL)
    exit_with_error("error calling default export", error, vm->trap);

  // Clean up after ourselves at this point
  wasmtime_module_delete(vm->module);
  wasmtime_store_delete(vm->store);
  wasm_engine_delete(vm->engine);
  return 0;
}

static void exit_with_error(const char *message, wasmtime_error_t *error,
                            wasm_trap_t *trap) {
  fprintf(stderr, "error: %s\n", message);
  wasm_byte_vec_t error_message;
  if (error != NULL) {
    wasmtime_error_message(error, &error_message);
    wasmtime_error_delete(error);
  } else {
    wasm_trap_message(trap, &error_message);
    wasm_trap_delete(trap);
  }
  fprintf(stderr, "%.*s\n", (int)error_message.size, error_message.data);
  wasm_byte_vec_delete(&error_message);
  exit(1);
}

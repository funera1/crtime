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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <wasm.h>
#include <wasmtime.h>

#include <cxxopts.hpp>
#include <ylt/struct_pack.hpp>

using namespace std;

static void exit_with_error(const char *message, wasmtime_error_t *error,
                            wasm_trap_t *trap);

bool write_binary(string filepath, uint8_t *data, size_t size){
    std::ofstream fout(filepath, std::ios::out | std::ios::binary);
    fout.write((char *)data, size);
    fout.close();
    return true;
}

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

  void get_memory() {
      wasmtime_instance_t instance = get_instance();
      wasmtime_extern_t export_;
      
      string name = "memory";
      bool ok = wasmtime_instance_export_get(context, &instance, name.c_str(), name.size(), &export_);
      if (!ok || export_.kind != WASMTIME_EXTERN_MEMORY) {
          printf("Failed to get memory export\n");
          return;
      }

      wasmtime_memory_t memory = export_.of.memory;
      printf("Memory retrieved successfully!\n");

      // メモリの情報を取得
      wasm_memorytype_t* memory_type = wasmtime_memory_type(context, &memory);
      const wasm_limits_t *limits = wasm_memorytype_limits(memory_type);
      printf("Memory min: %u, max: %u\n", limits->min, limits->max);

      // メモリの生データにアクセス
      uint8_t* data = wasmtime_memory_data(context, &memory);
      size_t size = wasmtime_memory_data_size(context, &memory);
      printf("Memory size: %zu bytes\n", size);

      // checkpoint memory
      // WasmState ws = WasmState{data, size};
      // std::vector<char> buffer = struct_pack::serialize(ws);
      if (!write_binary("wasm_memory.img", data, size)) {
        printf("failed to checkpoint memory");
      }
  }

};
VMCxt *vm = nullptr;

// SIGTRAP シグナルハンドラ
void sigtrap_handler(int sig) {
    printf("Caught SIGTRAP (signal number: %d)\n", sig);
    vm->get_memory();
}

void register_sigtrap() {
#if defined(_WIN32)
    signal(SIGILL, sigtrap_handler);
    // SPDLOG_DEBUG("SIGILL registered");
#else
    struct sigaction sa {};
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = sigtrap_handler;
    sa.sa_flags = SA_RESTART;

    // Register the signal handler for SIGTRAP
    if (sigaction(SIGTRAP, &sa, nullptr) == -1) {
        // SPDLOG_ERROR("Error: cannot handle SIGTRAP");
        exit(-1);
    }
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
  // spdlog::cfg::load_env_levels();

  // Parse options
  cxxopts::Options options("MyProgram", "One line description of MyProgram");
  options.add_options()
    ("d,debug", "Enable debugging") // a bool parameter
    ("i,integer", "Int param", cxxopts::value<int>())
    ("f,file", "File name", cxxopts::value<std::string>())
    ("v,verbose", "Verbose output", cxxopts::value<bool>()->default_value("false"))
    ;
  auto result = options.parse(argc, argv);
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
  wasmtime_module_delete(vm.module);
  wasmtime_store_delete(vm.store);
  wasm_engine_delete(vm.engine);
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

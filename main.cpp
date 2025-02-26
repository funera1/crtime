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
#include <string>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <wasm.h>
#include <wasmtime.h>

#include <cxxopts.hpp>

using namespace std;

static void exit_with_error(const char *message, wasmtime_error_t *error,
                            wasm_trap_t *trap);

// SIGTRAP シグナルハンドラ
void segfault_handler(int sig) {
    // auto end = std::chrono::high_resolution_clock::now();
    // auto dur = std::chronro::duration_cast<std::chrono::milliseconds>(end - wamr->time);
    // auto exec_env = wamr->get_exec_env();
    // print_exec_env_debug_info(exec_env);
    // print_memory(exec_env);
    // printf("Execution time: %f s\n", dur.count() / 1000000.0);
    // serialize_to_file(exec_env);
    // wamr->int3_cv.wait(wamr->int3_ul);
    // exit(EXIT_FAILURE);
    printf("Caught SEGFAULT (signal number: %d)\n", sig);
}
void sigtrap_handler(int sig) {
    printf("Caught SIGTRAP (signal number: %d)\n", sig);
    // ここで適切なデバッグやエラーハンドリングを行う
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

    struct sigaction sb {};
    sigemptyset(&sb.sa_mask);
    sb.sa_handler = segfault_handler;
    sb.sa_flags = SA_RESTART;

    // Register the signal handler for SIGTRAP
    if (sigaction(SIGTRAP, &sa, nullptr) == -1) {
        // SPDLOG_ERROR("Error: cannot handle SIGTRAP");
        exit(-1);
    } else {
        if (sigaction(SIGSYS, &sa, nullptr) == -1) {
            // SPDLOG_ERROR("Error: cannot handle SIGSYS");
            exit(-1);
        } else {
            if (sigaction(SIGSEGV, &sb, nullptr) == -1) {
                // SPDLOG_ERROR("Error: cannot handle SIGSEGV");
                exit(-1);
            } else {
                // SPDLOG_DEBUG("SIGSEGV registered");
            }
            // SPDLOG_DEBUG("SIGSYS registered");
        }
        // SPDLOG_DEBUG("SIGTRAP registered");
    }
#endif
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
  cout << file_name << endl;
  assert(file_name.size() != 0);

  // Register sigtrap handler
  register_sigtrap();

  // Set up our context
  wasm_config_t* config = wasm_config_new();
  wasmtime_config_strategy_set(config, WASMTIME_STRATEGY_WINCH);
  wasm_engine_t *engine = wasm_engine_new_with_config(config);
  assert(engine != NULL);
  wasmtime_store_t *store = wasmtime_store_new(engine, NULL, NULL);
  assert(store != NULL);
  wasmtime_context_t *context = wasmtime_store_context(store);

  // Create a linker with WASI functions defined
  wasmtime_linker_t *linker = wasmtime_linker_new(engine);
  wasmtime_error_t *error = wasmtime_linker_define_wasi(linker);
  if (error != NULL)
    exit_with_error("failed to link wasi", error, NULL);

  wasm_byte_vec_t wasm;
  // Load our input file to parse it next
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

  // Compile our modules
  wasmtime_module_t *module = NULL;
  error = wasmtime_module_new(engine, (uint8_t *)wasm.data, wasm.size, &module);
  if (!module)
    exit_with_error("failed to compile module", error, NULL);
  wasm_byte_vec_delete(&wasm);

  // Instantiate wasi
  wasi_config_t *wasi_config = wasi_config_new();
  assert(wasi_config);
  wasi_config_inherit_argv(wasi_config);
  wasi_config_inherit_env(wasi_config);
  wasi_config_inherit_stdin(wasi_config);
  wasi_config_inherit_stdout(wasi_config);
  wasi_config_inherit_stderr(wasi_config);
  wasm_trap_t *trap = NULL;
  error = wasmtime_context_set_wasi(context, wasi_config);
  if (error != NULL)
    exit_with_error("failed to instantiate WASI", error, NULL);

  // Instantiate the module
  error = wasmtime_linker_module(linker, context, "", 0, module);
  if (error != NULL)
    exit_with_error("failed to instantiate module", error, NULL);

  // Run it.
  wasmtime_func_t func;
  error = wasmtime_linker_get_default(linker, context, "", 0, &func);
  if (error != NULL)
    exit_with_error("failed to locate default export for module", error, NULL);

  error = wasmtime_func_call(context, &func, NULL, 0, NULL, 0, &trap);
  if (error != NULL || trap != NULL)
    exit_with_error("error calling default export", error, trap);

  // Clean up after ourselves at this point
  wasmtime_module_delete(module);
  wasmtime_store_delete(store);
  wasm_engine_delete(engine);
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

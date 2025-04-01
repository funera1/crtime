#include <wasmtime.h>
#include <spdlog/spdlog.h>
#include <string>

void exit_with_error(std::string message, wasmtime_error_t *error,
                            wasm_trap_t *trap) {
  spdlog::error("{:s}", message);
  wasm_byte_vec_t emsg;
  if (error != NULL) {
    wasmtime_error_message(error, &emsg);
    wasmtime_error_delete(error);
  } else {
    wasm_trap_message(trap, &emsg);
    wasm_trap_delete(trap);
  }

  std::string wasmtime_error_msg(emsg.data, (int)(emsg.size));
  spdlog::error("{:s}", wasmtime_error_msg);
  wasm_byte_vec_delete(&emsg);
  exit(1);
}

wasm_byte_vec_t load_wasm_file(std::string file_name) {
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
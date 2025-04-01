#include <wasmtime.h>
#include <spdlog/spdlog.h>
#include <string>

static void exit_with_error(std::string message, wasmtime_error_t *error,
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
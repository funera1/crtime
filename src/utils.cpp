#include <wasmtime.h>
#include <spdlog/spdlog.h>
#include <string>
#include <iostream>
#include <fstream>
#include <stdlib.h>

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

wasm_byte_vec_t load_wasm_from_file(std::string file_name) {
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
      spdlog::error("failed loading wasm code");
      exit(1);
  }
  fclose(file);
  return wasm;
}

wasm_byte_vec_t load_wasm_from_wasm(std::vector<uint8_t> code) {
  wasm_byte_vec_t wasm;
  wasm_byte_vec_new_uninitialized(&wasm, code.size());
  memcpy(wasm.data, code.data(), code.size());
  return wasm;
}

wasm_byte_vec_t load_wasm_from_wat(std::string wat) {
  wasmtime_error_t *error;
  wasm_byte_vec_t wasm;
  error = wasmtime_wat2wasm(wat.data(), wat.size(), &wasm);
  if (error != NULL) {
    exit_with_error("failed to wat2wasm", error, NULL);
  }
  
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

bool write_binary(std::string filepath, uint8_t *data, size_t size){
    std::ofstream fout(filepath, std::ios::out | std::ios::binary);
    fout.write((char *)data, size);
    fout.close();
    return true;
}

wasmtime_val_t* wasmtime_val_new(const wasm_valtype_vec_t *types) {
    size_t count = types->size;
    wasmtime_val_t* vals = (wasmtime_val_t *)calloc(count, sizeof(wasmtime_val_t));

    for (size_t i = 0; i < count; ++i) {
        wasm_valtype_t* valtype = types->data[i];
        wasm_valkind_t kind = wasm_valtype_kind(valtype);

        vals[i].kind = kind;

        // 初期値の設定（用途に応じて変えてOK）
        switch (kind) {
            case WASM_I32: vals[i].of.i32 = 0; break;
            case WASM_I64: vals[i].of.i64 = 0; break;
            case WASM_F32: vals[i].of.f32 = 0.0f; break;
            case WASM_F64: vals[i].of.f64 = 0.0; break;
            case WASM_EXTERNREF:
            case WASM_FUNCREF:
                spdlog::error("FUNCREF is unsupported");
                // vals[i].of.ref = NULL;
                break;
            default:
                spdlog::error("unsupported type");
                // 未対応の型
                break;
        }
    }

    return vals;
}
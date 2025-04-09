#include "vmcxt.h"
#include "utils.h"
#include "stack.h"
#include "restore.h"

#include <fmt/ranges.h>
#include <fstream>
#include <filesystem>

using namespace std;

uint32_t parse_pc(std::string dir) {
  const std::string path = dir + "wasm_pc.img";
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    spdlog::error("failed to open {:s}", path);
    return -1;
  }

  // read
  uint32_t value;
  file.read(reinterpret_cast<char*>(&value), sizeof(value));
  if (!file) {
    spdlog::error("failed to read {:s}", path);
    return -1;
  }
  
  return value;
}

using FileInfo = pair<streamsize, ifstream>;

optional<FileInfo> open_file(std::string path) {
  // open
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    spdlog::error("failed to open {:s}", path);
    return nullopt;
  }

  // ファイルサイズを取得
  file.seekg(0, std::ios::end);
  std::streamsize file_size = file.tellg();
  file.seekg(0, std::ios::beg);
  
  return make_pair(file_size, move(file));
}

Stack parse_stack(std::string dir) {
  const std::string path = dir + "wasm_stack.img";
  auto file_info = open_file(path);
  if (!file_info) {
    spdlog::error("failed to open file");
    return Stack();
  }
  auto [size, file] = move(file_info.value());

  // vectorにファイルの内容を格納
  std::vector<char> buffer(size);
  if (!file.read(buffer.data(), size)) {
      spdlog::error("failed to read {}", path);
  }
  
  // deseriaize
  auto result = struct_pack::deserialize<Stack>(buffer);
  if (!result) {
    auto error = result.error();
    spdlog::error("failed to deserialze stack: {}", struct_pack::error_message(error));
    return Stack();
  }

  return result.value();
}

Locals parse_locals(string dir) {
  // open file
  const std::string path = dir + "wasm_locals.img";
  auto file_info = open_file(path);
  if (!file_info) {
    spdlog::error("failed to open file");
    return Locals();
  }
  auto [size, file] = move(file_info.value());
  
  /// restore locals
  // vectorにファイルの内容を格納
  std::vector<char> buffer(size);
  if (!file.read(buffer.data(), size)) {
      spdlog::error("failed to read {}", path);
  }
  
  // deseriaize
  auto result = struct_pack::deserialize<Locals>(buffer);
  if (!result) {
    auto error = result.error();
    spdlog::error("failed to deserialze locals: {}", struct_pack::error_message(error));
    return Locals();
  }

  return result.value();
}

Memory parse_memory(string dir) {
  const std::string path = dir + "wasm_memory.img";
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        spdlog::error("Failed to open {:s}", path);
        exit(1);
    }

    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        spdlog::error("Failed to read wasm_memory.img");
        exit(1);
    }
    file.close();

    uint8_t* data = buffer.data();
    
    return Memory(data, size);
}

void set_restore_info(wasm_config_t* config, RestoreOption opt) {
    if (!opt.is_restore) {
      return;
    }

    // parse program counterfile
    uint32_t pc = parse_pc(opt.state_path);
    if (pc == -1) {
      spdlog::error("failed to restore pc");
      return;
    }
    spdlog::debug("restored wasm_pc: {:d}", pc);
    
    // parse stack file
    Stack stack = parse_stack(opt.state_path);
    spdlog::debug("restored stack: [{}]", fmt::join(stack.values, ", "));
    
    wasmtime_stack_t wasm_stack = wasmtime_stack_t {
      len: stack.values.size(),
      values: stack.values.data(),
      metadata: stack.metadata.data(),
    };

    // parse locals file
    Locals locals = parse_locals(opt.state_path);
    spdlog::debug("restored locals: [{}]", fmt::join(locals.values, ", "));
    wasmtime_locals_t wasm_locals = wasmtime_locals_t {
      len: locals.values.size(),
      types: locals.types.data(),
      values: locals.values.data(),
    };
    
    wasmtime_restore_info_t restore_info {
      is_restore: opt.is_restore,
      wasm_pc: pc,
      wasm_stack: wasm_stack,
      wasm_locals: wasm_locals,
    };
    
    wasmtime_config_set_restore_info(config, &restore_info);
}

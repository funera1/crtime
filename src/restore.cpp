#include "vmcxt.h"
#include "utils.h"
#include "stack.h"
#include "restore.h"

#include <fstream>

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

Stack parse_stack(std::string dir) {
  const std::string path = dir + "wasm_stack.img";
  // open
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    spdlog::error("failed to open {:s}", path);
    return Stack();
  }

  // ファイルサイズを取得
  file.seekg(0, std::ios::end);
  std::streamsize file_size = file.tellg();
  file.seekg(0, std::ios::beg);

  // vectorにファイルの内容を格納
  std::vector<char> buffer(file_size);
  if (!file.read(buffer.data(), file_size)) {
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
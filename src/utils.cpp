#include "utils.hpp"

#include <SDL3/SDL.h>
#include <cstdint>
#include <fstream>
#include <sstream>

void showError(SDL_Window *window, const std::string &error_message) {}

std::string readTextFile(const std::string &file_path) {
  std::ifstream infile(file_path);
  if (infile.is_open()) {
    std::stringstream buffer;
    buffer << infile.rdbuf();
    const std::string output{buffer.str()};
    infile.close();
    return output;
  }

  return std::string();
}

std::vector<uint32_t> readBinaryFile(const std::string &file_path) {
  std::ifstream file(file_path, std::ios::ate | std::ios::binary);
  if (!file.is_open())
    return {};
  size_t file_size{(size_t)file.tellg()};
  std::vector<uint32_t> buffer(file_size / sizeof(uint32_t));
  file.seekg(0);
  file.read((char *)buffer.data(), file_size);
  file.close();

  return buffer;
}

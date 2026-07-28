#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace asset {

struct BinaryAsset {
  std::string path;
  std::vector<uint8_t> data;
};

class AssetManager {
 public:
  static std::vector<std::string> candidate_paths(const std::vector<std::string>& relative_paths);
  static std::vector<std::string> existing_paths(const std::vector<std::string>& relative_paths);

  static bool read_binary(const std::string& relative_path, BinaryAsset& asset);

 private:
  AssetManager() = delete;
};

}  // namespace asset

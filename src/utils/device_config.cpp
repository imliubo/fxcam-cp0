#include "utils/device_config.h"

#include <cstdlib>

#include "utils/json_helper.h"

namespace util {
namespace {

constexpr CameraResolutionConfig kDefaultResolution{1280, 720};

bool is_supported_resolution(int width, int height) {
  return (width == 1280 && height == 720) || (width == 640 && height == 480);
}

}  // namespace

std::string device_config_path() {
  const char* home = std::getenv("HOME");
  return std::string(home && home[0] ? home : "/root") + "/.config/cardputerzero/config.json";
}

CameraResolutionConfig load_camera_resolution_config(const std::string& path) {
  Json config;
  if (!config.load_file(path.empty() ? device_config_path() : path)) {
    return kDefaultResolution;
  }

  const JsonValue resolution = config["camera"]["resolution"];
  if (!resolution["width"].is_number() || !resolution["height"].is_number()) {
    return kDefaultResolution;
  }
  const int width  = resolution["width"].as_int(kDefaultResolution.width);
  const int height = resolution["height"].as_int(kDefaultResolution.height);
  return is_supported_resolution(width, height) ? CameraResolutionConfig{width, height}
                                                : kDefaultResolution;
}

}  // namespace util

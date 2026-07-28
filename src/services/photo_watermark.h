#pragma once

#include <cstdint>
#include <ctime>
#include <string>
#include <vector>

#include "services/camera_filter.h"

namespace service {

struct PhotoWatermarkInfo {
  std::string device_name;
  std::string timestamp;
  std::string app_name;
  std::string aperture;
  std::string filter;
  std::string signature;
};

PhotoWatermarkInfo make_photo_watermark_info(CameraFilter filter,
                                             const std::string& aperture,
                                             std::time_t captured_at = std::time(nullptr));
std::vector<std::string> photo_watermark_lines(const PhotoWatermarkInfo& info);
void apply_photo_watermark_rgb888(std::vector<uint8_t>& pixels,
                                  int width,
                                  int height,
                                  const PhotoWatermarkInfo& info);

}  // namespace service

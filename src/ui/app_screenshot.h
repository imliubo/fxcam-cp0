#pragma once

#include <src/core/lv_obj.h>

#include <cstdint>
#include <string>

namespace ui {

struct ScreenshotResult {
  bool saved{false};
  std::string path;
  std::string error;
};

bool save_bgr888_png(const std::string& path,
                     const uint8_t* pixels,
                     uint32_t width,
                     uint32_t height,
                     uint32_t stride,
                     std::string& error);

ScreenshotResult capture_app_screenshot(lv_obj_t* root);
void show_screenshot_status(lv_obj_t* parent, const ScreenshotResult& result);

}  // namespace ui

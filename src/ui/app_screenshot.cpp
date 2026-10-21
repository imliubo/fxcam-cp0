#include "ui/app_screenshot.h"

#include <png.h>
#include <src/core/lv_obj_pos.h>
#include <src/core/lv_obj_style_gen.h>
#include <src/draw/lv_draw_buf.h>
#include <src/draw/snapshot/lv_snapshot.h>
#include <src/widgets/label/lv_label.h>

#include <array>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <system_error>

#include "ui/app_color.h"
#include "ui/app_font.h"
#include "utils/logger.h"

namespace ui {
namespace {

constexpr uint32_t kScreenshotWidth  = 320;
constexpr uint32_t kScreenshotHeight = 170;
constexpr uint32_t kStatusVisibleMs  = 1800;

std::filesystem::path screenshot_directory(std::string& error) {
  if (const char* override_dir = std::getenv("FXCAM_SCREENSHOT_DIR")) {
    if (override_dir[0] != '\0') {
      return override_dir;
    }
  }

  const char* home = std::getenv("HOME");
  if (!home || home[0] == '\0') {
    error = "HOME is not set";
    return {};
  }
  return std::filesystem::path(home) / "Pictures" / "Screenshots" / "FXCam";
}

std::string timestamp_now() {
  const std::time_t now = std::time(nullptr);
  std::tm local_time{};
#if defined(_WIN32)
  localtime_s(&local_time, &now);
#else
  localtime_r(&now, &local_time);
#endif
  std::array<char, 32> timestamp{};
  std::strftime(timestamp.data(), timestamp.size(), "%Y%m%d_%H%M%S", &local_time);
  return timestamp.data();
}

std::filesystem::path available_path(const std::filesystem::path& directory) {
  const std::string stem          = "FXCam_" + timestamp_now();
  std::filesystem::path candidate = directory / (stem + ".png");
  std::error_code ec;
  if (!std::filesystem::exists(candidate, ec)) {
    return candidate;
  }

  for (uint32_t suffix = 1; suffix <= 999999; ++suffix) {
    std::array<char, 16> suffix_text{};
    std::snprintf(suffix_text.data(), suffix_text.size(), "-%02u", suffix);
    candidate = directory / (stem + suffix_text.data() + ".png");
    ec.clear();
    if (!std::filesystem::exists(candidate, ec)) {
      return candidate;
    }
  }
  return {};
}

}  // namespace

bool save_bgr888_png(const std::string& path,
                     const uint8_t* pixels,
                     uint32_t width,
                     uint32_t height,
                     uint32_t stride,
                     std::string& error) {
  error.clear();
  if (path.empty() || !pixels || width == 0 || height == 0 || stride < width * 3U) {
    error = "Invalid screenshot image data";
    return false;
  }

  png_image image{};
  image.version = PNG_IMAGE_VERSION;
  image.width   = width;
  image.height  = height;
  image.format  = PNG_FORMAT_BGR;
  if (!png_image_write_to_file(&image,
                               path.c_str(),
                               0,
                               pixels,
                               static_cast<png_int_32>(stride),
                               nullptr)) {
    error = image.message[0] ? image.message : "libpng failed to write the screenshot";
    png_image_free(&image);
    std::error_code ec;
    std::filesystem::remove(path, ec);
    return false;
  }

  png_image_free(&image);
  return true;
}

ScreenshotResult capture_app_screenshot(lv_obj_t* root) {
  ScreenshotResult result;
  if (!root) {
    result.error = "No active screen";
    LOG_ERROR("Screenshot failed: {}", result.error);
    return result;
  }

  lv_obj_update_layout(root);
  lv_draw_buf_t* snapshot = lv_snapshot_take(root, LV_COLOR_FORMAT_RGB888);
  if (!snapshot) {
    result.error = "Unable to capture the current screen";
    LOG_ERROR("Screenshot failed: {}", result.error);
    return result;
  }

  const uint32_t width  = snapshot->header.w;
  const uint32_t height = snapshot->header.h;
  if (width != kScreenshotWidth || height != kScreenshotHeight) {
    result.error =
        "Expected 320x170 screen, got " + std::to_string(width) + "x" + std::to_string(height);
    lv_draw_buf_destroy(snapshot);
    LOG_ERROR("Screenshot failed: {}", result.error);
    return result;
  }

  const std::filesystem::path directory = screenshot_directory(result.error);
  if (directory.empty()) {
    lv_draw_buf_destroy(snapshot);
    LOG_ERROR("Screenshot failed: {}", result.error);
    return result;
  }

  std::error_code ec;
  std::filesystem::create_directories(directory, ec);
  if (ec) {
    result.error = "Unable to create screenshot directory: " + ec.message();
    lv_draw_buf_destroy(snapshot);
    LOG_ERROR("Screenshot failed: {}", result.error);
    return result;
  }

  const std::filesystem::path output_path = available_path(directory);
  if (output_path.empty()) {
    result.error = "Unable to allocate a unique screenshot filename";
    lv_draw_buf_destroy(snapshot);
    LOG_ERROR("Screenshot failed: {}", result.error);
    return result;
  }
  result.path  = output_path.string();
  result.saved = save_bgr888_png(result.path,
                                 snapshot->data,
                                 width,
                                 height,
                                 snapshot->header.stride,
                                 result.error);
  lv_draw_buf_destroy(snapshot);

  if (result.saved) {
    LOG_INFO("Screenshot saved: {}", result.path);
  } else {
    LOG_ERROR("Screenshot failed for {}: {}", result.path, result.error);
  }
  return result;
}

void show_screenshot_status(lv_obj_t* parent, const ScreenshotResult& result) {
  if (!parent) {
    return;
  }

  lv_obj_t* label = lv_label_create(parent);
  lv_label_set_text(label, result.saved ? "Screenshot saved" : "Screenshot failed");
  lv_obj_set_style_text_font(label, font::AppFont::inter_semibold(11), 0);
  lv_obj_set_style_text_color(label, lv_color_hex(color::DARK_ONSURFACE), 0);
  lv_obj_set_style_bg_color(label, lv_color_hex(color::DARK_SURFACECONTAINERHIGH), 0);
  lv_obj_set_style_bg_opa(label, LV_OPA_90, 0);
  lv_obj_set_style_border_width(label, 1, 0);
  lv_obj_set_style_border_color(label, lv_color_hex(color::DARK_OUTLINEVARIANT), 0);
  lv_obj_set_style_radius(label, 5, 0);
  lv_obj_set_style_pad_hor(label, 8, 0);
  lv_obj_set_style_pad_ver(label, 5, 0);
  lv_obj_set_style_shadow_width(label, 0, 0);
  lv_obj_align(label, LV_ALIGN_TOP_RIGHT, -8, 8);
  lv_obj_delete_delayed(label, kStatusVisibleMs);
}

}  // namespace ui

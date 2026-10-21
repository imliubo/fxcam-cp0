#include <png.h>

#include <array>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "lvgl.h"
#include "ui/app_screenshot.h"

int main() {
  const std::filesystem::path output_path =
      std::filesystem::temp_directory_path() / "fxcam_screenshot_test.png";
  const std::array<uint8_t, 16> bgr_pixels{{
      0,
      0,
      255,
      0,
      255,
      0,
      99,
      99,
      255,
      0,
      0,
      255,
      255,
      255,
      88,
      88,
  }};

  std::string error;
  assert(ui::save_bgr888_png(output_path.string(), bgr_pixels.data(), 2, 2, 8, error));

  png_image image{};
  image.version = PNG_IMAGE_VERSION;
  assert(png_image_begin_read_from_file(&image, output_path.string().c_str()));
  assert(image.width == 2);
  assert(image.height == 2);
  image.format = PNG_FORMAT_RGB;

  std::array<uint8_t, 12> rgb_pixels{};
  assert(png_image_finish_read(&image, nullptr, rgb_pixels.data(), 0, nullptr));
  png_image_free(&image);

  assert((rgb_pixels == std::array<uint8_t, 12>{{
                            255,
                            0,
                            0,
                            0,
                            255,
                            0,
                            0,
                            0,
                            255,
                            255,
                            255,
                            255,
                        }}));

  std::error_code ec;
  std::filesystem::remove(output_path, ec);

  const std::filesystem::path screenshot_dir =
      std::filesystem::temp_directory_path() / "fxcam_screenshot_capture_test";
  std::filesystem::create_directories(screenshot_dir, ec);
  assert(!ec);
  assert(setenv("FXCAM_SCREENSHOT_DIR", screenshot_dir.string().c_str(), 1) == 0);

  lv_init();
  lv_display_t* display = lv_display_create(320, 170);
  assert(display);
  lv_obj_t* screen = lv_screen_active();
  assert(screen);
  lv_obj_set_style_bg_color(screen, lv_color_hex(0xFF0000), 0);
  lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

  const ui::ScreenshotResult result = ui::capture_app_screenshot(screen);
  assert(result.saved);
  assert(std::filesystem::exists(result.path));
  const ui::ScreenshotResult second_result = ui::capture_app_screenshot(screen);
  assert(second_result.saved);
  assert(second_result.path != result.path);
  assert(std::filesystem::exists(second_result.path));

  png_image captured{};
  captured.version = PNG_IMAGE_VERSION;
  assert(png_image_begin_read_from_file(&captured, result.path.c_str()));
  assert(captured.width == 320);
  assert(captured.height == 170);
  captured.format = PNG_FORMAT_RGB;
  std::vector<uint8_t> captured_pixels(PNG_IMAGE_SIZE(captured));
  assert(png_image_finish_read(&captured, nullptr, captured_pixels.data(), 0, nullptr));
  png_image_free(&captured);
  assert(captured_pixels[0] == 255);
  assert(captured_pixels[1] == 0);
  assert(captured_pixels[2] == 0);

  lv_display_delete(display);
  lv_deinit();
  std::filesystem::remove(result.path, ec);
  std::filesystem::remove(second_result.path, ec);
  std::filesystem::remove(screenshot_dir, ec);
  unsetenv("FXCAM_SCREENSHOT_DIR");
  std::cout << "screenshot_test passed\n";
  return 0;
}

#include "services/photo_watermark.h"

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

namespace {

void test_lines_include_all_capture_information() {
  const service::PhotoWatermarkInfo info{"CardputerZero",
                                         "2026-07-28 12:34:56",
                                         "FXCam",
                                         "F/2.0",
                                         "GAME BOY",
                                         "@imliubo"};
  const auto lines = service::photo_watermark_lines(info);
  assert(lines.size() == 3);
  assert(lines[0] == "CardputerZero");
  assert(lines[1] == "2026-07-28 12:34:56");
  assert(lines[2] == "FXCam  F/2.0  GAME BOY  @imliubo");
}

void test_renderer_only_modifies_bottom_left_region() {
  constexpr int width       = 640;
  constexpr int height      = 480;
  constexpr uint8_t initial = 120;
  std::vector<uint8_t> pixels(static_cast<size_t>(width * height * 3), initial);
  const service::PhotoWatermarkInfo info{"CardputerZero",
                                         "2026-07-28 12:34:56",
                                         "FXCam",
                                         "F/2.0",
                                         "GAME BOY",
                                         "@imliubo"};

  service::apply_photo_watermark_rgb888(pixels, width, height, info);

  bool bottom_left_changed = false;
  for (int y = height - 80; y < height - 20; ++y) {
    for (int x = 20; x < 340; ++x) {
      const size_t index = static_cast<size_t>(y * width + x) * 3;
      bottom_left_changed |= pixels[index] != initial;
    }
  }
  assert(bottom_left_changed);

  const size_t bottom_margin = static_cast<size_t>((height - 10) * width + 30) * 3;
  const size_t left_margin   = static_cast<size_t>((height - 60) * width + 10) * 3;
  assert(pixels[bottom_margin] == initial);
  assert(pixels[left_margin] == initial);

  const size_t former_panel_corner = static_cast<size_t>((height - 59) * width + 26) * 3;
  assert(pixels[former_panel_corner] == initial);
  assert(pixels[former_panel_corner + 1] == initial);
  assert(pixels[former_panel_corner + 2] == initial);

  const size_t top_right = static_cast<size_t>(20 * width + width - 20) * 3;
  assert(pixels[top_right] == initial);
  assert(pixels[top_right + 1] == initial);
  assert(pixels[top_right + 2] == initial);
}

void test_renderer_draws_lowercase_p_in_device_name() {
  constexpr int width       = 640;
  constexpr int height      = 480;
  constexpr uint8_t initial = 120;
  std::vector<uint8_t> pixels(static_cast<size_t>(width * height * 3), initial);
  const service::PhotoWatermarkInfo info{"p", "", "", "", "", ""};

  service::apply_photo_watermark_rgb888(pixels, width, height, info);

  // At this resolution the first glyph starts at (30, 425). A lowercase p has
  // a blank top row and a set left pixel on row two; the fallback '?' does not.
  const size_t blank_top_pixel = static_cast<size_t>(425 * width + 31) * 3;
  const size_t set_stem_pixel  = static_cast<size_t>(427 * width + 30) * 3;
  assert(pixels[blank_top_pixel] == initial);
  assert(pixels[set_stem_pixel] == 255);
}

}  // namespace

int main() {
  test_lines_include_all_capture_information();
  test_renderer_only_modifies_bottom_left_region();
  test_renderer_draws_lowercase_p_in_device_name();
  return 0;
}

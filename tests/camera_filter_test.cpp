#include "services/camera_filter.h"

#include <cassert>
#include <cstdint>
#include <set>
#include <string>
#include <vector>

int main() {
  using service::CameraFilter;

  const CameraFilter last_filter =
      static_cast<CameraFilter>(static_cast<int>(CameraFilter::Count) - 1);
  assert(service::step_camera_filter(CameraFilter::Original, -1) == last_filter);
  assert(service::step_camera_filter(last_filter, 1) == CameraFilter::Original);
  assert(service::step_camera_filter(CameraFilter::Polaroid, 1) == CameraFilter::GameBoy);
  assert(static_cast<int>(CameraFilter::Count) == 106);

  std::set<std::string> filter_names;
  for (int value = 0; value < static_cast<int>(CameraFilter::Count); ++value) {
    filter_names.insert(service::camera_filter_name(static_cast<CameraFilter>(value)));
  }
  assert(filter_names.size() == static_cast<size_t>(CameraFilter::Count));

  std::vector<uint8_t> original = {
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
  };
  auto untouched = original;
  service::apply_camera_filter_rgb888(untouched, 2, 2, CameraFilter::Original);
  assert(untouched == original);

  auto mono = original;
  service::apply_camera_filter_rgb888(mono, 2, 2, CameraFilter::Mono);
  for (size_t i = 0; i < mono.size(); i += 3) {
    assert(mono[i] == mono[i + 1]);
    assert(mono[i + 1] == mono[i + 2]);
  }

  std::vector<uint8_t> gradient(8 * 8 * 3);
  for (size_t i = 0; i < gradient.size(); i += 3) {
    const uint8_t value = static_cast<uint8_t>((i / 3) * 255 / 63);
    gradient[i] = gradient[i + 1] = gradient[i + 2] = value;
  }
  service::apply_camera_filter_rgb888(gradient, 8, 8, CameraFilter::GameBoy);
  std::set<uint32_t> colors;
  for (size_t i = 0; i < gradient.size(); i += 3) {
    colors.insert((static_cast<uint32_t>(gradient[i]) << 16) |
                  (static_cast<uint32_t>(gradient[i + 1]) << 8) | gradient[i + 2]);
  }
  assert(colors.size() >= 2 && colors.size() <= 4);

  std::vector<uint8_t> detailed_gameboy(1280 * 2 * 3, 0);
  for (int y = 0; y < 2; ++y) {
    for (int x = 4; x < 8; ++x) {
      const size_t index          = static_cast<size_t>(y * 1280 + x) * 3;
      detailed_gameboy[index]     = 255;
      detailed_gameboy[index + 1] = 255;
      detailed_gameboy[index + 2] = 255;
    }
  }
  service::apply_camera_filter_rgb888(detailed_gameboy, 1280, 2, CameraFilter::GameBoy);
  const size_t dark_index   = 0;
  const size_t bright_index = 4 * 3;
  assert(detailed_gameboy[dark_index] != detailed_gameboy[bright_index] ||
         detailed_gameboy[dark_index + 1] != detailed_gameboy[bright_index + 1] ||
         detailed_gameboy[dark_index + 2] != detailed_gameboy[bright_index + 2]);

  auto polaroid = original;
  service::apply_camera_filter_rgb888(polaroid, 2, 2, CameraFilter::Polaroid);
  assert(polaroid != original);

  const std::vector<uint8_t> effect_source = {
      24,  80, 160, 210, 45,  120, 75,  190, 55, 235, 210, 130,
      140, 35, 220, 35,  145, 205, 190, 115, 40, 90,  55,  170,
  };
  for (CameraFilter filter : {CameraFilter::VividWarm,
                              CameraFilter::VividCool,
                              CameraFilter::Dramatic,
                              CameraFilter::DramaticWarm,
                              CameraFilter::DramaticCool,
                              CameraFilter::Silvertone,
                              CameraFilter::Comic,
                              CameraFilter::Neon,
                              CameraFilter::Thermal,
                              CameraFilter::Crt,
                              CameraFilter::Noir,
                              CameraFilter::Solarize}) {
    auto filtered = effect_source;
    service::apply_camera_filter_rgb888(filtered, 4, 2, filter);
    assert(filtered != effect_source);
  }

  std::vector<uint8_t> neutral = {128, 128, 128};
  auto vivid_warm              = neutral;
  auto vivid_cool              = neutral;
  service::apply_camera_filter_rgb888(vivid_warm, 1, 1, CameraFilter::VividWarm);
  service::apply_camera_filter_rgb888(vivid_cool, 1, 1, CameraFilter::VividCool);
  assert(vivid_warm[0] > vivid_warm[2]);
  assert(vivid_cool[2] > vivid_cool[0]);

  auto silvertone = effect_source;
  service::apply_camera_filter_rgb888(silvertone, 4, 2, CameraFilter::Silvertone);
  for (size_t i = 0; i < silvertone.size(); i += 3) {
    assert(silvertone[i] == silvertone[i + 1]);
    assert(silvertone[i + 1] == silvertone[i + 2]);
  }

  std::vector<uint16_t> rgb565(16, 0xffff);
  service::apply_camera_filter_rgb565(rgb565, 4, 4, CameraFilter::GameBoy);
  assert(rgb565.front() != 0xffff);

  std::vector<uint8_t> palette_source(16 * 16 * 3);
  for (size_t pixel = 0; pixel < palette_source.size() / 3; ++pixel) {
    const uint8_t value           = static_cast<uint8_t>(pixel * 255 / 255);
    palette_source[pixel * 3]     = value;
    palette_source[pixel * 3 + 1] = value;
    palette_source[pixel * 3 + 2] = value;
  }
  for (int value = static_cast<int>(CameraFilter::PaletteFirst);
       value < static_cast<int>(CameraFilter::Count);
       ++value) {
    auto filtered = palette_source;
    service::apply_camera_filter_rgb888(filtered, 16, 16, static_cast<CameraFilter>(value));
    assert(filtered != palette_source);
  }

  for (int value = static_cast<int>(CameraFilter::PaletteFirst);
       value < static_cast<int>(CameraFilter::Count);
       ++value) {
    std::vector<uint16_t> palette_rgb565(16 * 16);
    for (size_t pixel = 0; pixel < palette_rgb565.size(); ++pixel) {
      const uint16_t gray = static_cast<uint16_t>(pixel * 255 / 255);
      palette_rgb565[pixel] =
          static_cast<uint16_t>(((gray & 0xf8) << 8) | ((gray & 0xfc) << 3) | (gray >> 3));
    }
    const auto rgb565_source = palette_rgb565;
    service::apply_camera_filter_rgb565(palette_rgb565, 16, 16, static_cast<CameraFilter>(value));
    assert(palette_rgb565 != rgb565_source);
  }
  return 0;
}

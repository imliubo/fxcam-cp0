#include "services/retro_palette_filter.h"

#include <algorithm>
#include <array>
#include <cstdio>

namespace service::retro_palette {
namespace {

struct Rgb {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

struct PaletteTheme {
  const char* name;
  std::array<Rgb, 4> colors;
};

constexpr std::array<PaletteTheme, kThemeCount> kThemes{{
    {"DMG", {{{15, 56, 15}, {48, 98, 48}, {139, 172, 15}, {155, 188, 15}}}},
    {"POCKET", {{{30, 32, 20}, {86, 90, 68}, {155, 158, 132}, {218, 220, 190}}}},
    {"VIRTUAL", {{{20, 0, 0}, {90, 0, 0}, {190, 12, 12}, {255, 85, 55}}}},
    {"NES", {{{15, 15, 35}, {45, 80, 170}, {210, 55, 70}, {248, 224, 170}}}},
    {"FAMICOM", {{{30, 20, 25}, {120, 30, 35}, {210, 120, 55}, {246, 225, 186}}}},
    {"SNES", {{{22, 18, 42}, {76, 66, 140}, {158, 125, 195}, {230, 220, 242}}}},
    {"GENESIS", {{{8, 15, 35}, {28, 95, 120}, {165, 50, 120}, {245, 200, 90}}}},
    {"ARCADE", {{{8, 8, 18}, {0, 135, 145}, {235, 45, 110}, {255, 235, 105}}}},
    {"CGA", {{{0, 0, 0}, {0, 170, 170}, {170, 0, 170}, {255, 255, 255}}}},
    {"EGA", {{{0, 0, 0}, {0, 85, 170}, {170, 85, 0}, {255, 255, 85}}}},
    {"ZX", {{{0, 0, 0}, {0, 0, 205}, {205, 0, 0}, {255, 255, 255}}}},
    {"C64", {{{25, 25, 112}, {111, 61, 134}, {117, 162, 69}, {170, 255, 238}}}},
    {"AMIGA", {{{18, 14, 31}, {53, 96, 130}, {196, 81, 69}, {246, 226, 178}}}},
    {"MSX", {{{0, 0, 0}, {33, 96, 168}, {183, 70, 90}, {235, 220, 90}}}},
    {"PICO8", {{{0, 0, 0}, {41, 173, 255}, {255, 0, 77}, {255, 241, 232}}}},
    {"ENDESGA", {{{27, 38, 50}, {35, 103, 104}, {208, 70, 72}, {255, 231, 132}}}},
    {"ICE", {{{5, 16, 35}, {20, 85, 125}, {82, 185, 205}, {220, 250, 245}}}},
    {"LAVA", {{{25, 2, 8}, {110, 15, 10}, {235, 75, 15}, {255, 235, 95}}}},
    {"FOREST", {{{8, 25, 15}, {25, 75, 35}, {105, 145, 55}, {225, 230, 155}}}},
    {"OCEAN", {{{3, 15, 38}, {5, 75, 105}, {20, 165, 170}, {190, 240, 225}}}},
    {"SUNSET", {{{30, 10, 45}, {125, 35, 95}, {235, 95, 75}, {255, 218, 145}}}},
    {"GRAPE", {{{20, 8, 35}, {82, 35, 110}, {170, 80, 150}, {245, 200, 225}}}},
}};

uint16_t pack_rgb565(const Rgb& color) {
  return static_cast<uint16_t>(((color.r & 0xf8) << 8) | ((color.g & 0xfc) << 3) | (color.b >> 3));
}

int rgb565_luminance(uint16_t pixel) {
  const int red   = ((pixel >> 11) & 0x1f) * 255 / 31;
  const int green = ((pixel >> 5) & 0x3f) * 255 / 63;
  const int blue  = (pixel & 0x1f) * 255 / 31;
  return (77 * red + 150 * green + 29 * blue) >> 8;
}

int rgb888_luminance(const std::vector<uint8_t>& pixels, int width, int x, int y) {
  const size_t offset = static_cast<size_t>(y * width + x) * 3;
  return (77 * pixels[offset] + 150 * pixels[offset + 1] + 29 * pixels[offset + 2]) >> 8;
}

int block_size_for(int width, int height, int variant) {
  if (variant < 2) {
    return std::max(1, std::max(width / 640, height / 480));
  }
  if (variant == 2) {
    return std::max(2, std::max(width / 320, height / 240));
  }
  return std::max(2, std::max(width / 200, height / 160));
}

int palette_index_for(int luma, int block_x, int block_y, int variant) {
  constexpr int bayer2[2][2] = {{-24, 8}, {24, -8}};
  constexpr int bayer4[4][4] = {
      {-30, 2, -22, 10},
      {18, -14, 26, -6},
      {-18, 14, -26, 6},
      {30, -2, 22, -10},
  };

  int adjusted = luma;
  if (variant == 1) {
    adjusted += bayer2[block_y & 1][block_x & 1];
  } else if (variant == 2) {
    adjusted += bayer4[block_y & 3][block_x & 3];
  } else if (variant == 3) {
    adjusted = (adjusted - 128) * 125 / 100 + 128;
    adjusted += (block_y & 1) ? 18 : -18;
    adjusted += ((block_x * 17 + block_y * 31) & 15) - 7;
  }
  return std::min(3, std::max(0, std::min(255, adjusted)) * 4 / 256);
}

template <typename Pixel, typename ReadLuminance, typename WritePixel>
void apply_impl(std::vector<Pixel>& pixels,
                int width,
                int height,
                int index,
                ReadLuminance read_luminance,
                WritePixel write_pixel) {
  if (index < 0 || index >= kFilterCount || width <= 0 || height <= 0 ||
      pixels.size() < static_cast<size_t>(width) * height) {
    return;
  }

  const int theme_index = index / kVariantCount;
  const int variant     = index % kVariantCount;
  const int block_size  = block_size_for(width, height, variant);
  const auto& palette   = kThemes[static_cast<size_t>(theme_index)].colors;

  for (int top = 0; top < height; top += block_size) {
    for (int left = 0; left < width; left += block_size) {
      const int bottom = std::min(height, top + block_size);
      const int right  = std::min(width, left + block_size);
      int luma_sum     = 0;
      int count        = 0;
      for (int y = top; y < bottom; ++y) {
        for (int x = left; x < right; ++x) {
          luma_sum += read_luminance(pixels, width, x, y);
          ++count;
        }
      }

      const Rgb& color = palette[static_cast<size_t>(palette_index_for(count ? luma_sum / count : 0,
                                                                       left / block_size,
                                                                       top / block_size,
                                                                       variant))];
      for (int y = top; y < bottom; ++y) {
        for (int x = left; x < right; ++x) {
          write_pixel(pixels, width, x, y, color);
        }
      }
    }
  }
}

}  // namespace

const char* filter_name(int index) {
  static const auto names = [] {
    std::array<std::array<char, 16>, kFilterCount> result{};
    for (int theme = 0; theme < kThemeCount; ++theme) {
      for (int variant = 0; variant < kVariantCount; ++variant) {
        std::snprintf(result[static_cast<size_t>(theme * kVariantCount + variant)].data(),
                      result.front().size(),
                      "%s-%d",
                      kThemes[static_cast<size_t>(theme)].name,
                      variant + 1);
      }
    }
    return result;
  }();
  return index >= 0 && index < kFilterCount ? names[static_cast<size_t>(index)].data() : "PALETTE";
}

void apply_rgb565(std::vector<uint16_t>& pixels, int width, int height, int index) {
  apply_impl(
      pixels,
      width,
      height,
      index,
      [](const std::vector<uint16_t>& source, int source_width, int x, int y) {
        return rgb565_luminance(source[static_cast<size_t>(y * source_width + x)]);
      },
      [](std::vector<uint16_t>& output, int output_width, int x, int y, const Rgb& color) {
        output[static_cast<size_t>(y * output_width + x)] = pack_rgb565(color);
      });
}

void apply_rgb888(std::vector<uint8_t>& pixels, int width, int height, int index) {
  if (pixels.size() < static_cast<size_t>(std::max(0, width)) * std::max(0, height) * 3) {
    return;
  }
  apply_impl(
      pixels,
      width,
      height,
      index,
      [](const std::vector<uint8_t>& source, int source_width, int x, int y) {
        return rgb888_luminance(source, source_width, x, y);
      },
      [](std::vector<uint8_t>& output, int output_width, int x, int y, const Rgb& color) {
        const size_t offset = static_cast<size_t>(y * output_width + x) * 3;
        output[offset]      = color.r;
        output[offset + 1]  = color.g;
        output[offset + 2]  = color.b;
      });
}

}  // namespace service::retro_palette

#include "services/camera_filter.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>

#include "services/retro_palette_filter.h"

namespace service {
namespace {

static_assert(kRetroPaletteFilterCount == retro_palette::kFilterCount);

std::atomic<CameraFilter> g_active_filter{CameraFilter::Original};

uint8_t clamp_u8(int value) { return static_cast<uint8_t>(std::max(0, std::min(255, value))); }

uint16_t pack_rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return static_cast<uint16_t>(((r & 0xf8) << 8) | ((g & 0xfc) << 3) | (b >> 3));
}

void unpack_rgb565(uint16_t pixel, uint8_t& r, uint8_t& g, uint8_t& b) {
  r = static_cast<uint8_t>(((pixel >> 11) & 0x1f) * 255 / 31);
  g = static_cast<uint8_t>(((pixel >> 5) & 0x3f) * 255 / 63);
  b = static_cast<uint8_t>((pixel & 0x1f) * 255 / 31);
}

int luminance(uint8_t r, uint8_t g, uint8_t b) { return (77 * r + 150 * g + 29 * b) >> 8; }

bool is_retro_palette_filter(CameraFilter filter) {
  const int value = static_cast<int>(filter);
  return value >= static_cast<int>(CameraFilter::PaletteFirst) &&
         value < static_cast<int>(CameraFilter::Count);
}

int retro_palette_index(CameraFilter filter) {
  return static_cast<int>(filter) - static_cast<int>(CameraFilter::PaletteFirst);
}

uint8_t quantize_comic_channel(int value) {
  if (value < 48) {
    return 0;
  }
  if (value < 112) {
    return 85;
  }
  if (value < 192) {
    return 170;
  }
  return 255;
}

uint8_t mix_channel(int from, int to, int amount, int range) {
  return clamp_u8(from + (to - from) * amount / range);
}

void thermal_color(int luma, uint8_t& r, uint8_t& g, uint8_t& b) {
  const int segment = std::min(3, luma / 64);
  const int amount  = luma - segment * 64;
  constexpr std::array<std::array<uint8_t, 3>, 5> palette{{
      {{4, 0, 18}},
      {{55, 0, 150}},
      {{245, 20, 35}},
      {{255, 220, 0}},
      {{255, 255, 245}},
  }};
  r = mix_channel(palette[segment][0], palette[segment + 1][0], amount, 64);
  g = mix_channel(palette[segment][1], palette[segment + 1][1], amount, 64);
  b = mix_channel(palette[segment][2], palette[segment + 1][2], amount, 64);
}

void apply_vivid_style(int source_r,
                       int source_g,
                       int source_b,
                       int temperature,
                       uint8_t& r,
                       uint8_t& g,
                       uint8_t& b) {
  const int gray        = luminance(source_r, source_g, source_b);
  const int saturated_r = gray + (source_r - gray) * 135 / 100;
  const int saturated_g = gray + (source_g - gray) * 135 / 100;
  const int saturated_b = gray + (source_b - gray) * 135 / 100;
  r                     = clamp_u8((saturated_r - 128) * 118 / 100 + 128 + temperature);
  g                     = clamp_u8((saturated_g - 128) * 118 / 100 + 128 + temperature / 4);
  b                     = clamp_u8((saturated_b - 128) * 118 / 100 + 128 - temperature);
}

void apply_dramatic_style(int source_r,
                          int source_g,
                          int source_b,
                          int temperature,
                          uint8_t& r,
                          uint8_t& g,
                          uint8_t& b) {
  const int gray       = luminance(source_r, source_g, source_b);
  const int contrasted = (gray - 128) * 145 / 100 + 128;
  r                    = clamp_u8(contrasted + (source_r - gray) * 88 / 100 + temperature);
  g                    = clamp_u8(contrasted + (source_g - gray) * 88 / 100 + temperature / 4);
  b                    = clamp_u8(contrasted + (source_b - gray) * 88 / 100 - temperature);
}

void transform_pixel(CameraFilter filter,
                     int x,
                     int y,
                     int width,
                     int height,
                     uint8_t& r,
                     uint8_t& g,
                     uint8_t& b) {
  const int source_r = r;
  const int source_g = g;
  const int source_b = b;

  switch (filter) {
    case CameraFilter::Polaroid: {
      const int grain = ((x * 17 + y * 31) & 7) - 3;
      r               = clamp_u8((source_r * 92 + 2550) / 100 + 8 + grain);
      g               = clamp_u8((source_g * 91 + 2295) / 100 + 4 + grain);
      b               = clamp_u8((source_b * 84 + 2040) / 100 + grain);

      const int dx       = width > 1 ? std::abs(2 * x - width + 1) * 256 / width : 0;
      const int dy       = height > 1 ? std::abs(2 * y - height + 1) * 256 / height : 0;
      const int vignette = std::max(205, 256 - (dx * dx + dy * dy) / 900);
      r                  = static_cast<uint8_t>(r * vignette / 256);
      g                  = static_cast<uint8_t>(g * vignette / 256);
      b                  = static_cast<uint8_t>(b * vignette / 256);
      break;
    }
    case CameraFilter::Mono: {
      const uint8_t gray = static_cast<uint8_t>(luminance(r, g, b));
      r                  = gray;
      g                  = gray;
      b                  = gray;
      break;
    }
    case CameraFilter::Vintage: {
      const int sepia_r = (393 * source_r + 769 * source_g + 189 * source_b) / 1351;
      const int sepia_g = (349 * source_r + 686 * source_g + 168 * source_b) / 1203;
      const int sepia_b = (272 * source_r + 534 * source_g + 131 * source_b) / 1080;
      r                 = clamp_u8((sepia_r * 82 + 2550) / 100);
      g                 = clamp_u8((sepia_g * 78 + 2295) / 100);
      b                 = clamp_u8((sepia_b * 70 + 1785) / 100);
      break;
    }
    case CameraFilter::Vivid:
      apply_vivid_style(source_r, source_g, source_b, 0, r, g, b);
      break;
    case CameraFilter::VividWarm:
      apply_vivid_style(source_r, source_g, source_b, 12, r, g, b);
      break;
    case CameraFilter::VividCool:
      apply_vivid_style(source_r, source_g, source_b, -12, r, g, b);
      break;
    case CameraFilter::Dramatic:
      apply_dramatic_style(source_r, source_g, source_b, 0, r, g, b);
      break;
    case CameraFilter::DramaticWarm:
      apply_dramatic_style(source_r, source_g, source_b, 10, r, g, b);
      break;
    case CameraFilter::DramaticCool:
      apply_dramatic_style(source_r, source_g, source_b, -10, r, g, b);
      break;
    case CameraFilter::Silvertone: {
      const uint8_t gray = clamp_u8((luminance(r, g, b) - 128) * 125 / 100 + 136);
      r                  = gray;
      g                  = gray;
      b                  = gray;
      break;
    }
    case CameraFilter::Comic: {
      const int gray = luminance(r, g, b);
      if (gray < 42) {
        r = 8;
        g = 8;
        b = 12;
        break;
      }
      r = quantize_comic_channel(gray + (source_r - gray) * 165 / 100);
      g = quantize_comic_channel(gray + (source_g - gray) * 165 / 100);
      b = quantize_comic_channel(gray + (source_b - gray) * 165 / 100);
      break;
    }
    case CameraFilter::Neon: {
      const int neon_r = (source_r * 135 + source_b * 55) / 100 - 24;
      const int neon_g = (source_g * 120 + source_b * 28) / 100 - 22;
      const int neon_b = (source_b * 140 + source_r * 45) / 100 + 18;
      r                = clamp_u8((neon_r - 128) * 125 / 100 + 128);
      g                = clamp_u8((neon_g - 128) * 125 / 100 + 128);
      b                = clamp_u8((neon_b - 128) * 125 / 100 + 128);
      break;
    }
    case CameraFilter::Thermal:
      thermal_color(luminance(r, g, b), r, g, b);
      break;
    case CameraFilter::Crt: {
      int crt_r = (source_r - 128) * 110 / 100 + 128;
      int crt_g = (source_g - 128) * 110 / 100 + 128;
      int crt_b = (source_b - 128) * 110 / 100 + 128;
      if (x % 3 == 0) {
        crt_r = crt_r * 112 / 100;
        crt_g = crt_g * 90 / 100;
        crt_b = crt_b * 90 / 100;
      } else if (x % 3 == 1) {
        crt_r = crt_r * 90 / 100;
        crt_g = crt_g * 112 / 100;
        crt_b = crt_b * 90 / 100;
      } else {
        crt_r = crt_r * 90 / 100;
        crt_g = crt_g * 90 / 100;
        crt_b = crt_b * 112 / 100;
      }
      const int scanline = (y & 1) ? 70 : 100;
      r                  = clamp_u8(crt_r * scanline / 100);
      g                  = clamp_u8(crt_g * scanline / 100);
      b                  = clamp_u8(crt_b * scanline / 100);
      break;
    }
    case CameraFilter::Noir: {
      const int grain = ((x * 13 + y * 29) & 15) - 7;
      const int gray  = clamp_u8((luminance(r, g, b) - 128) * 160 / 100 + 128 + grain);
      r               = static_cast<uint8_t>(gray);
      g               = static_cast<uint8_t>(gray);
      b               = static_cast<uint8_t>(gray);
      break;
    }
    case CameraFilter::Solarize:
      r = clamp_u8(source_r < 128 ? source_r * 2 : (255 - source_r) * 2);
      g = clamp_u8(source_g < 128 ? source_g * 2 : (255 - source_g) * 2);
      b = clamp_u8(source_b < 128 ? source_b * 2 : (255 - source_b) * 2);
      break;
    case CameraFilter::Original:
    case CameraFilter::GameBoy:
    case CameraFilter::PaletteFirst:
    case CameraFilter::Count:
      break;
  }
}

constexpr std::array<std::array<uint8_t, 3>, 4> kGameBoyPalette{{
    {{15, 56, 15}},
    {{48, 98, 48}},
    {{139, 172, 15}},
    {{155, 188, 15}},
}};

int gameboy_block_size(int width, int height) {
  if (width > 400 || height > 300) {
    return std::max(1, std::max(width / 640, height / 560));
  }
  return std::max(2, std::max(width / 128, height / 112));
}

int gameboy_palette_index(int luma, int block_x, int block_y, bool detailed) {
  constexpr int bayer2[2][2] = {{-18, 10}, {18, -10}};
  constexpr int bayer4[4][4] = {
      {-30, 2, -22, 10},
      {18, -14, 26, -6},
      {-18, 14, -26, 6},
      {30, -2, 22, -10},
  };
  const int offset = detailed ? bayer4[block_y & 3][block_x & 3] : bayer2[block_y & 1][block_x & 1];
  const int dithered = std::max(0, std::min(255, luma + offset));
  return std::min(3, dithered * 4 / 256);
}

void apply_gameboy_rgb888(std::vector<uint8_t>& pixels, int width, int height) {
  const int block_size = gameboy_block_size(width, height);
  const bool detailed  = width > 400 || height > 300;
  for (int top = 0; top < height; top += block_size) {
    for (int left = 0; left < width; left += block_size) {
      const int bottom = std::min(height, top + block_size);
      const int right  = std::min(width, left + block_size);
      int luma_sum     = 0;
      int count        = 0;
      for (int y = top; y < bottom; ++y) {
        for (int x = left; x < right; ++x) {
          const size_t index = static_cast<size_t>(y * width + x) * 3;
          luma_sum += luminance(pixels[index], pixels[index + 1], pixels[index + 2]);
          ++count;
        }
      }
      const auto& color = kGameBoyPalette[gameboy_palette_index(count ? luma_sum / count : 0,
                                                                left / block_size,
                                                                top / block_size,
                                                                detailed)];
      for (int y = top; y < bottom; ++y) {
        for (int x = left; x < right; ++x) {
          const size_t index = static_cast<size_t>(y * width + x) * 3;
          pixels[index]      = color[0];
          pixels[index + 1]  = color[1];
          pixels[index + 2]  = color[2];
        }
      }
    }
  }
}

void apply_gameboy_rgb565(std::vector<uint16_t>& pixels, int width, int height) {
  const int block_size = gameboy_block_size(width, height);
  const bool detailed  = width > 400 || height > 300;
  for (int top = 0; top < height; top += block_size) {
    for (int left = 0; left < width; left += block_size) {
      const int bottom = std::min(height, top + block_size);
      const int right  = std::min(width, left + block_size);
      int luma_sum     = 0;
      int count        = 0;
      for (int y = top; y < bottom; ++y) {
        for (int x = left; x < right; ++x) {
          uint8_t r = 0;
          uint8_t g = 0;
          uint8_t b = 0;
          unpack_rgb565(pixels[static_cast<size_t>(y * width + x)], r, g, b);
          luma_sum += luminance(r, g, b);
          ++count;
        }
      }
      const auto& color     = kGameBoyPalette[gameboy_palette_index(count ? luma_sum / count : 0,
                                                                    left / block_size,
                                                                    top / block_size,
                                                                    detailed)];
      const uint16_t packed = pack_rgb565(color[0], color[1], color[2]);
      for (int y = top; y < bottom; ++y) {
        std::fill(pixels.begin() + static_cast<size_t>(y * width + left),
                  pixels.begin() + static_cast<size_t>(y * width + right),
                  packed);
      }
    }
  }
}

}  // namespace

const char* camera_filter_name(CameraFilter filter) {
  if (is_retro_palette_filter(filter)) {
    return retro_palette::filter_name(retro_palette_index(filter));
  }
  switch (filter) {
    case CameraFilter::Original:
      return "ORIGINAL";
    case CameraFilter::Polaroid:
      return "POLAROID";
    case CameraFilter::GameBoy:
      return "GAME BOY";
    case CameraFilter::Mono:
      return "MONO";
    case CameraFilter::Vintage:
      return "VINTAGE";
    case CameraFilter::Vivid:
      return "VIVID";
    case CameraFilter::VividWarm:
      return "VIVID WARM";
    case CameraFilter::VividCool:
      return "VIVID COOL";
    case CameraFilter::Dramatic:
      return "DRAMATIC";
    case CameraFilter::DramaticWarm:
      return "DRAMATIC WARM";
    case CameraFilter::DramaticCool:
      return "DRAMATIC COOL";
    case CameraFilter::Silvertone:
      return "SILVERTONE";
    case CameraFilter::Comic:
      return "COMIC";
    case CameraFilter::Neon:
      return "NEON";
    case CameraFilter::Thermal:
      return "THERMAL";
    case CameraFilter::Crt:
      return "CRT";
    case CameraFilter::Noir:
      return "NOIR";
    case CameraFilter::Solarize:
      return "SOLARIZE";
    case CameraFilter::PaletteFirst:
      return retro_palette::filter_name(0);
    case CameraFilter::Count:
      return "ORIGINAL";
  }
  return "ORIGINAL";
}

CameraFilter step_camera_filter(CameraFilter filter, int direction) {
  const int count = static_cast<int>(CameraFilter::Count);
  int value       = static_cast<int>(filter);
  value           = (value + (direction < 0 ? -1 : 1) + count) % count;
  return static_cast<CameraFilter>(value);
}

void set_active_camera_filter(CameraFilter filter) {
  if (filter == CameraFilter::Count) {
    filter = CameraFilter::Original;
  }
  g_active_filter.store(filter, std::memory_order_relaxed);
}

CameraFilter active_camera_filter() { return g_active_filter.load(std::memory_order_relaxed); }

void apply_camera_filter_rgb565(std::vector<uint16_t>& pixels,
                                int width,
                                int height,
                                CameraFilter filter) {
  if (filter == CameraFilter::Original || filter == CameraFilter::Count || width <= 0 ||
      height <= 0 || pixels.size() < static_cast<size_t>(width * height)) {
    return;
  }
  if (filter == CameraFilter::GameBoy) {
    apply_gameboy_rgb565(pixels, width, height);
    return;
  }
  if (is_retro_palette_filter(filter)) {
    retro_palette::apply_rgb565(pixels, width, height, retro_palette_index(filter));
    return;
  }

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const size_t index = static_cast<size_t>(y * width + x);
      uint8_t r          = 0;
      uint8_t g          = 0;
      uint8_t b          = 0;
      unpack_rgb565(pixels[index], r, g, b);
      transform_pixel(filter, x, y, width, height, r, g, b);
      pixels[index] = pack_rgb565(r, g, b);
    }
  }
}

void apply_camera_filter_rgb888(std::vector<uint8_t>& pixels,
                                int width,
                                int height,
                                CameraFilter filter) {
  if (filter == CameraFilter::Original || filter == CameraFilter::Count || width <= 0 ||
      height <= 0 || pixels.size() < static_cast<size_t>(width * height * 3)) {
    return;
  }
  if (filter == CameraFilter::GameBoy) {
    apply_gameboy_rgb888(pixels, width, height);
    return;
  }
  if (is_retro_palette_filter(filter)) {
    retro_palette::apply_rgb888(pixels, width, height, retro_palette_index(filter));
    return;
  }

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const size_t index = static_cast<size_t>(y * width + x) * 3;
      transform_pixel(filter,
                      x,
                      y,
                      width,
                      height,
                      pixels[index],
                      pixels[index + 1],
                      pixels[index + 2]);
    }
  }
}

}  // namespace service

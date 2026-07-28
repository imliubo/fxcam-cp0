#include "services/photo_watermark.h"

#include <algorithm>
#include <array>
#include <cstdio>

namespace service {
namespace {

using Glyph = std::array<uint8_t, 7>;

Glyph glyph_for(char character) {
  switch (character) {
    case 'A':
      return {14, 17, 17, 31, 17, 17, 17};
    case 'B':
      return {30, 17, 17, 30, 17, 17, 30};
    case 'C':
      return {14, 17, 16, 16, 16, 17, 14};
    case 'D':
      return {30, 17, 17, 17, 17, 17, 30};
    case 'E':
      return {31, 16, 16, 30, 16, 16, 31};
    case 'F':
      return {31, 16, 16, 30, 16, 16, 16};
    case 'G':
      return {14, 17, 16, 23, 17, 17, 15};
    case 'H':
      return {17, 17, 17, 31, 17, 17, 17};
    case 'I':
      return {31, 4, 4, 4, 4, 4, 31};
    case 'J':
      return {7, 2, 2, 2, 18, 18, 12};
    case 'K':
      return {17, 18, 20, 24, 20, 18, 17};
    case 'L':
      return {16, 16, 16, 16, 16, 16, 31};
    case 'M':
      return {17, 27, 21, 21, 17, 17, 17};
    case 'N':
      return {17, 25, 21, 19, 17, 17, 17};
    case 'O':
      return {14, 17, 17, 17, 17, 17, 14};
    case 'P':
      return {30, 17, 17, 30, 16, 16, 16};
    case 'Q':
      return {14, 17, 17, 17, 21, 18, 13};
    case 'R':
      return {30, 17, 17, 30, 20, 18, 17};
    case 'S':
      return {15, 16, 16, 14, 1, 1, 30};
    case 'T':
      return {31, 4, 4, 4, 4, 4, 4};
    case 'U':
      return {17, 17, 17, 17, 17, 17, 14};
    case 'V':
      return {17, 17, 17, 17, 17, 10, 4};
    case 'W':
      return {17, 17, 17, 21, 21, 21, 10};
    case 'X':
      return {17, 17, 10, 4, 10, 17, 17};
    case 'Y':
      return {17, 17, 10, 4, 4, 4, 4};
    case 'Z':
      return {31, 1, 2, 4, 8, 16, 31};
    case 'a':
      return {0, 0, 14, 1, 15, 17, 15};
    case 'b':
      return {16, 16, 30, 17, 17, 17, 30};
    case 'c':
      return {0, 0, 14, 16, 16, 17, 14};
    case 'd':
      return {1, 1, 15, 17, 17, 17, 15};
    case 'e':
      return {0, 0, 14, 17, 31, 16, 14};
    case 'i':
      return {4, 0, 12, 4, 4, 4, 14};
    case 'l':
      return {12, 4, 4, 4, 4, 4, 14};
    case 'm':
      return {0, 0, 26, 21, 21, 21, 21};
    case 'o':
      return {0, 0, 14, 17, 17, 17, 14};
    case 'r':
      return {0, 0, 22, 25, 16, 16, 16};
    case 't':
      return {8, 8, 30, 8, 8, 9, 6};
    case 'u':
      return {0, 0, 17, 17, 17, 19, 13};
    case '0':
      return {14, 17, 19, 21, 25, 17, 14};
    case '1':
      return {4, 12, 4, 4, 4, 4, 14};
    case '2':
      return {14, 17, 1, 2, 4, 8, 31};
    case '3':
      return {30, 1, 1, 14, 1, 1, 30};
    case '4':
      return {2, 6, 10, 18, 31, 2, 2};
    case '5':
      return {31, 16, 16, 30, 1, 1, 30};
    case '6':
      return {14, 16, 16, 30, 17, 17, 14};
    case '7':
      return {31, 1, 2, 4, 8, 8, 8};
    case '8':
      return {14, 17, 17, 14, 17, 17, 14};
    case '9':
      return {14, 17, 17, 15, 1, 1, 14};
    case '-':
      return {0, 0, 0, 31, 0, 0, 0};
    case ':':
      return {0, 4, 4, 0, 4, 4, 0};
    case '.':
      return {0, 0, 0, 0, 0, 4, 4};
    case '/':
      return {1, 1, 2, 4, 8, 16, 16};
    case '@':
      return {14, 17, 23, 21, 23, 16, 14};
    case ' ':
      return {0, 0, 0, 0, 0, 0, 0};
    default:
      return {14, 17, 1, 2, 4, 0, 4};
  }
}

void blend_pixel(std::vector<uint8_t>& pixels,
                 int width,
                 int height,
                 int x,
                 int y,
                 uint8_t red,
                 uint8_t green,
                 uint8_t blue,
                 int opacity) {
  if (x < 0 || x >= width || y < 0 || y >= height) {
    return;
  }
  const size_t index = static_cast<size_t>(y * width + x) * 3;
  pixels[index] = static_cast<uint8_t>((pixels[index] * (255 - opacity) + red * opacity) / 255);
  pixels[index + 1] =
      static_cast<uint8_t>((pixels[index + 1] * (255 - opacity) + green * opacity) / 255);
  pixels[index + 2] =
      static_cast<uint8_t>((pixels[index + 2] * (255 - opacity) + blue * opacity) / 255);
}

void draw_text(std::vector<uint8_t>& pixels,
               int width,
               int height,
               int x,
               int y,
               int scale,
               const std::string& text) {
  int cursor_x = x;
  for (char character : text) {
    const Glyph glyph = glyph_for(character);
    for (int row = 0; row < 7; ++row) {
      for (int column = 0; column < 5; ++column) {
        if ((glyph[static_cast<size_t>(row)] & (1U << (4 - column))) == 0) {
          continue;
        }
        for (int dy = 0; dy < scale; ++dy) {
          for (int dx = 0; dx < scale; ++dx) {
            blend_pixel(pixels,
                        width,
                        height,
                        cursor_x + column * scale + dx,
                        y + row * scale + dy,
                        255,
                        255,
                        255,
                        255);
          }
        }
      }
    }
    cursor_x += 6 * scale;
  }
}

}  // namespace

PhotoWatermarkInfo make_photo_watermark_info(CameraFilter filter,
                                             const std::string& aperture,
                                             std::time_t captured_at) {
  std::tm local_time{};
#if defined(_WIN32)
  localtime_s(&local_time, &captured_at);
#else
  localtime_r(&captured_at, &local_time);
#endif
  char timestamp[20]{};
  std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &local_time);
  return {"CardputerZero", timestamp, "FXCam", aperture, camera_filter_name(filter), "@imliubo"};
}

std::vector<std::string> photo_watermark_lines(const PhotoWatermarkInfo& info) {
  return {info.device_name,
          info.timestamp,
          info.app_name + "  " + info.aperture + "  " + info.filter + "  " + info.signature};
}

void apply_photo_watermark_rgb888(std::vector<uint8_t>& pixels,
                                  int width,
                                  int height,
                                  const PhotoWatermarkInfo& info) {
  if (width <= 0 || height <= 0 ||
      pixels.size() < static_cast<size_t>(width) * static_cast<size_t>(height) * 3) {
    return;
  }

  const std::vector<std::string> lines = photo_watermark_lines(info);
  const int scale           = std::max(1, std::min(4, (std::min(width, height) + 240) / 480));
  constexpr int kTextMargin = 30;
  const int line_advance    = 9 * scale;
  size_t longest_line       = 0;
  for (const auto& line : lines) {
    longest_line = std::max(longest_line, line.size());
  }
  const int text_width  = static_cast<int>(longest_line) * 6 * scale - scale;
  const int text_height = static_cast<int>((lines.size() - 1) * line_advance + 7 * scale);
  const int text_x      = std::min(kTextMargin, std::max(0, width - text_width));
  const int text_y      = std::max(0, height - kTextMargin - text_height);

  for (size_t line = 0; line < lines.size(); ++line) {
    draw_text(pixels,
              width,
              height,
              text_x,
              text_y + static_cast<int>(line) * line_advance,
              scale,
              lines[line]);
  }
}

}  // namespace service

#pragma once

#include <cstdint>
#include <vector>

namespace service::retro_palette {

inline constexpr int kThemeCount   = 22;
inline constexpr int kVariantCount = 4;
inline constexpr int kFilterCount  = kThemeCount * kVariantCount;

const char* filter_name(int index);
void apply_rgb565(std::vector<uint16_t>& pixels, int width, int height, int index);
void apply_rgb888(std::vector<uint8_t>& pixels, int width, int height, int index);

}  // namespace service::retro_palette

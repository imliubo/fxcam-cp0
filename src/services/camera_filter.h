#pragma once

#include <cstdint>
#include <vector>

namespace service {

inline constexpr int kRetroPaletteFilterCount = 88;

enum class CameraFilter : uint8_t {
  Original = 0,
  Polaroid,
  GameBoy,
  Mono,
  Vintage,
  Vivid,
  VividWarm,
  VividCool,
  Dramatic,
  DramaticWarm,
  DramaticCool,
  Silvertone,
  Comic,
  Neon,
  Thermal,
  Crt,
  Noir,
  Solarize,
  PaletteFirst,
  Count = PaletteFirst + kRetroPaletteFilterCount,
};

const char* camera_filter_name(CameraFilter filter);
CameraFilter step_camera_filter(CameraFilter filter, int direction);

void set_active_camera_filter(CameraFilter filter);
CameraFilter active_camera_filter();

void apply_camera_filter_rgb565(std::vector<uint16_t>& pixels,
                                int width,
                                int height,
                                CameraFilter filter);
void apply_camera_filter_rgb888(std::vector<uint8_t>& pixels,
                                int width,
                                int height,
                                CameraFilter filter);

}  // namespace service

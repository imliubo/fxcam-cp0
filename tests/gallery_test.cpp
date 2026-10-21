// libjpeg expects size_t and FILE to be declared by its caller.
// clang-format off
#include <cstddef>
#include <cstdio>
#include <jpeglib.h>
// clang-format on

#include <lvgl.h>

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "services/app_services.h"
#include "services/camera_backend_utils.h"
#include "ui/gallery_jpeg_decoder.h"
#include "viewmodels/gallery_viewmodel.h"

namespace {

std::filesystem::path make_test_home() {
  return std::filesystem::temp_directory_path() /
         ("fxcam-gallery-" + std::to_string(static_cast<unsigned long>(std::rand())));
}

void test_full_jpeg_decode(const std::filesystem::path& photo_path) {
  constexpr int width  = 16;
  constexpr int height = 12;
  std::vector<uint8_t> pixels(static_cast<size_t>(width * height * 3));
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const size_t offset = static_cast<size_t>(y * width + x) * 3;
      pixels[offset]      = static_cast<uint8_t>(x * 12);
      pixels[offset + 1]  = static_cast<uint8_t>(y * 16);
      pixels[offset + 2]  = 80;
    }
  }

  assert(service::camera_backend::save_jpeg_rgb888(photo_path.string(), pixels, width, height, 90));
  auto decoded = ui::decode_gallery_jpeg(photo_path.string());
  assert(decoded);
  assert(decoded->header.w == width);
  assert(decoded->header.h == height);
  assert(decoded->header.cf == LV_COLOR_FORMAT_RGB888);
}

void test_gallery_detail_state(const std::filesystem::path& home) {
  auto services     = std::make_shared<service::AppServices>();
  services->gallery = std::make_shared<service::GalleryService>();
  viewmodel::GalleryViewModel viewmodel(services);

  viewmodel.on_enter();
  assert(viewmodel.snapshot().count == 1);
  assert(!viewmodel.detail_mode());

  assert(viewmodel.handle_action(app::AppAction::Confirm));
  assert(viewmodel.detail_mode());
  assert(viewmodel.snapshot().zoom_percent == 100);

  assert(viewmodel.handle_action(app::AppAction::ZoomIn));
  assert(viewmodel.snapshot().zoom_percent == 150);
  assert(viewmodel.handle_action(app::AppAction::PanRight));
  assert(viewmodel.handle_action(app::AppAction::PanDown));
  assert(viewmodel.snapshot().pan_x == 12);
  assert(viewmodel.snapshot().pan_y == 12);

  assert(viewmodel.handle_action(app::AppAction::ShowInfo));
  assert(viewmodel.snapshot().info_visible);
  assert(viewmodel.handle_action(app::AppAction::Exit));
  assert(!viewmodel.snapshot().info_visible);
  assert(viewmodel.detail_mode());

  assert(viewmodel.handle_action(app::AppAction::ToggleCaptureMode));
  assert(viewmodel.snapshot().zoom_percent == 100);
  assert(viewmodel.snapshot().pan_x == 0);
  assert(viewmodel.snapshot().pan_y == 0);

  for (int index = 0; index < 10; ++index) {
    assert(viewmodel.handle_action(app::AppAction::ZoomIn));
    assert(viewmodel.handle_action(app::AppAction::PanRight));
  }
  assert(viewmodel.snapshot().zoom_percent == 800);
  assert(viewmodel.snapshot().pan_x == 100);
  assert(viewmodel.handle_action(app::AppAction::ToggleCaptureMode));

  assert(viewmodel.handle_action(app::AppAction::Exit));
  assert(!viewmodel.detail_mode());
  assert(viewmodel.handle_action(app::AppAction::Exit));
  assert(viewmodel.consume_transition_request() == app::AppState::Camera);
  viewmodel.on_exit();

  std::filesystem::remove_all(home);
}

}  // namespace

int main() {
  lv_init();
  const auto home      = make_test_home();
  const auto photo_dir = home / "Pictures/DCIM/Camera";
  std::filesystem::create_directories(photo_dir);
  const auto photo_path = photo_dir / "gallery-detail.jpg";
  test_full_jpeg_decode(photo_path);

#if defined(_WIN32)
  _putenv_s("HOME", home.string().c_str());
#else
  setenv("HOME", home.string().c_str(), 1);
#endif
  test_gallery_detail_state(home);
  return 0;
}

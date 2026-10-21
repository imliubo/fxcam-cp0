#pragma once

#include <src/draw/lv_draw_buf.h>

#include <memory>
#include <string>

namespace ui {

struct GalleryDrawBufferDeleter {
  void operator()(lv_draw_buf_t* buffer) const noexcept;
};

using GalleryDrawBufferPtr = std::unique_ptr<lv_draw_buf_t, GalleryDrawBufferDeleter>;

GalleryDrawBufferPtr decode_gallery_jpeg(const std::string& path);

}  // namespace ui

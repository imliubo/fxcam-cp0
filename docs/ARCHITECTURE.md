# FXCam Architecture

## Runtime Layers

FXCam keeps LVGL ownership on the main thread and separates camera-specific work behind a
small service interface.

1. `src/input` converts Linux evdev or SDL keys into `app::AppAction` values.
2. `src/main.cpp` applies screen-specific key semantics and dispatches actions.
3. `src/viewmodels` updates services and publishes display state.
4. `src/services` owns camera backends, capture, filters, gallery indexing, metadata, and audio.
5. `src/views` owns LVGL objects, timers, animations, and subject bindings.

`Screen` tracks whether its ViewModel is active. Repeated `show()` or `hide()` calls are
idempotent so camera and gallery services receive one lifecycle callback per transition.

## Camera Data Flow

The CSI path is implemented by `LibcameraBackend`; the USB path is implemented by `V4l2Backend`.
Both implement `CameraInterface` and expose the same `CameraFrame`, capture, video, zoom, and state
operations to `CameraService`.

```text
Camera backend -> RGB565 preview buffer -> active filter -> CameraService
              -> CameraViewModel -> PreviewFrameLimiter -> CameraView -> LVGL image
```

Preview buffers come from `CameraFramePool`. Shared ownership prevents a backend from reusing a
buffer while LVGL still references it. `PreviewFrameLimiter` presents at a bounded interval and
coalesces stale frames instead of growing a queue.

Still capture uses an RGB888 buffer:

```text
Camera still frame -> decode/convert -> crop/resize -> active filter
                   -> text watermark -> EXIF/JPEG writer -> ~/Pictures/DCIM/Camera
```

The active filter is stored atomically because libcamera completion callbacks can run outside the
LVGL main loop. Preview and still capture read the same value.

## Filters

`CameraFilter` contains the original and iPhone-style effects followed by 88 generated retro
palette variants. `retro_palette_filter.cpp` defines 22 four-color themes. Each theme has clean,
2 x 2 Bayer, 4 x 4 Bayer, and coarse scanline/random variants.

Every filter must support both formats:

- RGB565 for the low-cost real-time preview.
- RGB888 for full-resolution JPEG output.

When adding a filter, update its enum value, display name, RGB565/RGB888 behavior, the expected
filter count, and the unique-name/format tests in `tests/camera_filter_test.cpp`.

## Input Semantics

The input drivers only translate physical keys. `src/main.cpp` then adapts numeric actions for the
active screen. This preserves the CardputerZero keypad layout while keeping ViewModels independent
of Linux key codes and SDL scan codes.

Letter shortcuts `Z/H` and `C/J` select the previous or next filter. Numeric keys retain different
camera and gallery meanings documented in the README files.

## Assets And Packaging

Runtime asset lookup checks explicit environment roots, configured roots, source-tree locations,
and `/usr/share/fxcam`. The Debian package installs:

```text
/usr/bin/fxcam
/usr/share/fxcam/{audio,fonts}
/usr/share/APPLaunch/applications/fxcam.desktop
/usr/share/APPLaunch/share/images/fxcam.png
```

`package_deb.sh` uses CPack's DEB generator when available. Its portable fallback stages the CMake
install tree, writes GNU-format `control.tar.xz` and `data.tar.xz` archives without host extended
attributes, and assembles the package with `ar`.

## Tests

- `camera_preview_test`: frame pooling, limiting, resize, and JPEG output.
- `camera_filter_test`: filter count, names, stepping, RGB565, RGB888, and key visual properties.
- `photo_watermark_test`: text content and bottom-left rendering bounds.
- `device_config_test`: supported resolution parsing and fallback behavior.
- `screen_lifecycle_test`: idempotent ViewModel enter/exit callbacks.

CMake explicitly undefines `NDEBUG` for test targets so these assertion-based tests remain active
in both Debug and Release configurations.

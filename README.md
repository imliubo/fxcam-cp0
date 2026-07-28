# FXCam

[中文说明](README_zh.md)

FXCam is a camera with retro effects for M5CardputerZero / CM0. It uses LVGL 9 for the
interface, libcamera for the CSI camera, V4L2 for USB cameras, and APPLaunch for device
integration. The project was initially based on the
[CardputerZero camera project](https://github.com/CardputerZero/CameraApp).

## Features

- Live RGB565 preview and full-resolution JPEG capture with matching filters.
- 106 selectable effects, including Polaroid, Game Boy, iPhone-style filters, CRT, thermal,
  comic, and 88 retro palette/dithering combinations.
- Detailed Game Boy processing for full-resolution photos.
- Transparent photo watermark with device name, local time, app name, aperture, active filter,
  and author signature.
- EXIF metadata for CSI-camera captures.
- Built-in gallery with image details and deletion confirmation.
- CSI camera through libcamera with automatic V4L2 USB-camera fallback.
- APPLaunch desktop entry, icon, arm64 Debian package, and direct ADB deployment.

Photos are saved under `~/Pictures/DCIM/Camera`. Video support writes MJPEG AVI files to
`~/Videos/Camera`, although the current device UI focuses on still photography.

## Controls

| Key | Camera action |
| --- | --- |
| `Fn + Left`, `Z`, `H`, `2`, `,` | Previous filter |
| `Fn + Right`, `C`, `J`, `3`, `.` | Next filter |
| `6`, `Enter` | Capture photo |
| `4`, `Esc` | Return / exit |
| `5`, `7` | Zoom out / in |
| `8` | Open gallery |
| `F`, `X` | Pan up / down while zoomed |
| `1`, `F1` | Toggle shortcut hint |
| `U` | Switch CSI / USB camera backend |

In the gallery, `4` returns to the camera, `5` and `7` select the previous or next image,
`6` opens image information, `8` requests deletion, and `Enter` confirms a dialog action.

## Requirements

- CMake 3.23 or newer and Ninja.
- A C++17 compiler.
- Desktop builds: SDL2, libpng, libjpeg, Freetype, zlib, and cJSON.
- Device builds: an `aarch64-linux-gnu` GCC/G++ cross compiler.

The cross-build preset downloads the CM0 BSP sysroot into `.cache/sdk_bsp-src` when it is not
already available. Set `CM0_ALLOW_FETCH_DEPS=OFF` and provide `CM0_SDK_ROOT` for offline builds.

## Desktop Build And Tests

Apple Silicon macOS:

```bash
cmake --preset darwin-arm64 -DFXCAM_BUILD_TESTS=ON
cmake --build --preset darwin-arm64-dbg
ctest --test-dir build/darwin-arm64 -C Debug --output-on-failure
```

x86-64 Linux:

```bash
cmake --preset linux-x86-64 -DFXCAM_BUILD_TESTS=ON
cmake --build --preset linux-x86-64-dbg
ctest --test-dir build/linux-x86-64 -C Debug --output-on-failure
```

The desktop target uses a generated camera frame, so filter and UI work can be tested without a
physical camera.

## CardputerZero Build

```bash
cmake --preset cp0-cross -DFXCAM_BUILD_TESTS=ON
cmake --build --preset cp0-cross-rel
```

The application binary is written to:

```text
build/cp0-cross/Release/fxcam
```

Build the Debian package with:

```bash
./package_deb.sh
```

On hosts where CPack includes its DEB generator, the script uses CPack. On macOS installations
without that generator, it creates the same Debian archive using the system `ar` and `tar` tools.
Packages are written to `dist/`.

GitHub Actions runs desktop Debug and Release tests for pushes and pull requests. Device releases
are created only through the manually dispatched `Build and release` workflow.

## Device Deployment

For a root-enabled ADB connection:

```bash
./deploy_adb.sh
```

An explicit binary path can be supplied as the first argument:

```bash
./deploy_adb.sh /path/to/fxcam
```

The script installs the binary, runtime assets, APPLaunch icon, and desktop entry under `/usr`.
Restart APPLaunch or reboot the device after deployment if its app list is already open.

For SSH-only devices, copy the generated `.deb` to the target and install it with
`sudo dpkg -i <package>`.

## Project Layout

```text
src/services/     camera backends, filters, capture, gallery, audio, metadata
src/viewmodels/   screen state and application actions
src/views/        LVGL views
src/input/        Linux evdev and SDL keyboard input
cmake/            cross toolchain and Debian packaging
tests/            focused service and lifecycle tests
assets/           fonts, audio, and FXCam artwork
```

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for data flow, ownership boundaries, and
extension points.

## License

FXCam source code is licensed under the [MIT License](LICENSE). Bundled font assets retain
the terms in [assets/fonts/License.txt](assets/fonts/License.txt).

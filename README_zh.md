# FXCam

[English](README.md)

FXCam 是面向 M5CardputerZero / CM0 的复古滤镜相机应用，使用 LVGL 9 构建界面，
通过 libcamera 支持 CSI 摄像头，通过 V4L2 支持 USB 摄像头，并集成 APPLaunch。工程最初
参考了 [CardputerZero 相机工程](https://github.com/CardputerZero/CameraApp)。

## 功能

- 实时 RGB565 预览与全尺寸 JPEG 照片使用一致的滤镜。
- 106 种可切换效果，包括拍立得、Game Boy、iPhone 风格、CRT、热成像、漫画，以及
  88 种复古调色板和抖动组合。
- Game Boy 滤镜在全尺寸照片上使用更细的像素块和 4 x 4 有序抖动。
- 透明照片水印包含设备名、本地时间、应用名、光圈、当前滤镜和作者署名。
- CSI 摄像头照片包含 EXIF 信息。
- 内置图库支持图片信息查看和删除确认。
- CSI 摄像头使用 libcamera，USB 摄像头自动回退到 V4L2。
- 支持 APPLaunch 入口、应用图标、arm64 Debian 包和 ADB 直接部署。

照片保存到 `~/Pictures/DCIM/Camera`。视频模块将 MJPEG AVI 保存到 `~/Videos/Camera`，
但当前设备界面主要面向拍照操作。

## 按键

| 按键 | 相机界面 |
| --- | --- |
| `Fn + 左`、`Z`、`H`、`2`、`,` | 上一个滤镜 |
| `Fn + 右`、`C`、`J`、`3`、`.` | 下一个滤镜 |
| `6`、`Enter` | 拍照 |
| `4`、`Esc` | 返回或退出 |
| `5`、`7` | 缩小或放大 |
| `8` | 打开图库 |
| `F`、`X` | 放大时向上或向下平移 |
| `1`、`F1` | 显示或隐藏按键提示 |
| `U` | 切换 CSI / USB 摄像头 |

图库中，`4` 返回相机，`5` 和 `7` 切换上一张或下一张图片，`6` 查看图片信息，`8` 请求
删除，`Enter` 确认对话框操作。

## 环境要求

- CMake 3.23 或更高版本，以及 Ninja。
- 支持 C++17 的编译器。
- Desktop 构建需要 SDL2、libpng、libjpeg、Freetype、zlib 和 cJSON。
- 设备交叉编译需要 `aarch64-linux-gnu-gcc` 和 `aarch64-linux-gnu-g++`。

交叉编译预设会在缺少 BSP 时自动下载到 `.cache/sdk_bsp-src`。离线环境可设置
`CM0_ALLOW_FETCH_DEPS=OFF` 并通过 `CM0_SDK_ROOT` 指定现有 sysroot。

## Desktop 构建与测试

Apple Silicon macOS：

```bash
cmake --preset darwin-arm64 -DFXCAM_BUILD_TESTS=ON
cmake --build --preset darwin-arm64-dbg
ctest --test-dir build/darwin-arm64 -C Debug --output-on-failure
```

x86-64 Linux：

```bash
cmake --preset linux-x86-64 -DFXCAM_BUILD_TESTS=ON
cmake --build --preset linux-x86-64-dbg
ctest --test-dir build/linux-x86-64 -C Debug --output-on-failure
```

Desktop 模式使用动态测试画面，因此没有连接摄像头也可以检查滤镜和界面。

## CardputerZero 构建

```bash
cmake --preset cp0-cross -DFXCAM_BUILD_TESTS=ON
cmake --build --preset cp0-cross-rel
```

程序输出位置：

```text
build/cp0-cross/Release/fxcam
```

生成 Debian 安装包：

```bash
./package_deb.sh
```

当 CPack 提供 DEB 生成器时，脚本直接使用 CPack；在不提供该生成器的 macOS 环境中，
脚本会使用系统 `ar` 和 `tar` 生成相同结构的 Debian 包。安装包输出到 `dist/`。

GitHub Actions 会在推送和 Pull Request 时运行 Desktop Debug/Release 测试。设备版本仅通过
手动触发的 `Build and release` 工作流发布。

## 设备部署

ADB daemon 具有 root 权限时可以直接执行：

```bash
./deploy_adb.sh
```

也可以指定二进制路径：

```bash
./deploy_adb.sh /path/to/fxcam
```

脚本会把程序、运行资源、APPLaunch 图标和入口安装到 `/usr`。如果 APPLaunch 已经打开，
部署后需要重新启动 APPLaunch 或重启设备以刷新应用列表。

仅支持 SSH 的设备可以上传生成的 `.deb`，然后执行 `sudo dpkg -i <package>` 安装。

## 工程结构

```text
src/services/     摄像头后端、滤镜、拍摄、图库、音频和元数据
src/viewmodels/   页面状态和应用动作
src/views/        LVGL 页面
src/input/        Linux evdev 和 SDL 键盘输入
cmake/            交叉编译工具链和 Debian 打包
tests/            服务层与生命周期测试
assets/           字体、音效和 FXCam 图标源文件
```

数据流、所有权边界和扩展方式见 [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)。

## 许可证

FXCam 源代码使用 [MIT License](LICENSE)。字体资源继续遵循
[assets/fonts/License.txt](assets/fonts/License.txt) 中的许可条款。

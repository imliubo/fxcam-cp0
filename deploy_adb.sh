#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BINARY="${1:-${ROOT_DIR}/build/cp0-cross/Release/fxcam}"
ADB="${ADB:-adb}"

if [[ ! -f "${BINARY}" ]]; then
  echo "fxcam binary not found: ${BINARY}" >&2
  echo "Build it first with: cmake --build --preset cp0-cross-rel" >&2
  exit 1
fi

device_state="$("${ADB}" get-state 2>/dev/null || true)"
case "${device_state}" in
  device)
    ;;
  offline)
    echo "The device is offline. Reconnect USB, then run: adb kill-server && adb start-server" >&2
    exit 2
    ;;
  unauthorized)
    echo "The device has not authorized this computer for adb." >&2
    exit 2
    ;;
  *)
    echo "No usable adb device found (state: ${device_state:-unknown})." >&2
    echo "Check 'adb devices -l'; set ANDROID_SERIAL when multiple devices are attached." >&2
    exit 2
    ;;
esac

"${ADB}" root >/dev/null 2>&1 || true
"${ADB}" wait-for-device

device_uid="$("${ADB}" shell id -u | tr -d '\r')"
if [[ "${device_uid}" != "0" ]]; then
  echo "The device adb daemon must run as root to install APPLaunch files." >&2
  exit 1
fi

"${ADB}" shell mkdir -p \
  /usr/share/fxcam/audio \
  /usr/share/fxcam/fonts \
  /usr/share/APPLaunch/applications \
  /usr/share/APPLaunch/share/images

"${ADB}" push "${BINARY}" /usr/bin/fxcam
"${ADB}" shell chmod 0755 /usr/bin/fxcam
"${ADB}" push "${ROOT_DIR}/assets/audio/." /usr/share/fxcam/audio/
"${ADB}" push "${ROOT_DIR}/assets/fonts/." /usr/share/fxcam/fonts/
"${ADB}" push "${ROOT_DIR}/assets/images/fxcam.png" \
  /usr/share/APPLaunch/share/images/fxcam.png
"${ADB}" push "${ROOT_DIR}/deploy/fxcam.desktop" \
  /usr/share/APPLaunch/applications/fxcam.desktop
"${ADB}" shell chmod 0644 \
  /usr/share/APPLaunch/applications/fxcam.desktop \
  /usr/share/APPLaunch/share/images/fxcam.png
# Remove files created by releases published under the previous names.
"${ADB}" shell rm -f \
  /usr/bin/pocket_pixel \
  /usr/share/APPLaunch/applications/pocket_pixel.desktop \
  /usr/share/APPLaunch/share/images/pocket_pixel.png \
  /usr/bin/filter_camera \
  /usr/share/APPLaunch/applications/filter_camera.desktop \
  /usr/share/APPLaunch/share/images/filter_camera.png
"${ADB}" shell rm -rf /usr/share/pocket_pixel
"${ADB}" shell sync

echo "FXCam deployed. Restart APPLaunch or reboot the device to refresh the app list."

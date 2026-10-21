#pragma once

namespace app {

enum class AppAction {
  None,
  Exit,
  ToggleHint,
  Capture,
  Screenshot,
  Confirm,
  Delete,
  ZoomOut,
  ZoomIn,
  OpenGallery,
  ShowInfo,
  ToggleCaptureMode,
  ToggleCameraBackend,
  PreviousFilter,
  NextFilter,
  PanUp,
  PanDown,
  PanLeft,
  PanRight,
};

}  // namespace app

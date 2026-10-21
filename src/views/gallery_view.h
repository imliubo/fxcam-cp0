#pragma once

#include <array>
#include <string>

#include "ui/gallery_jpeg_decoder.h"
#include "viewmodels/gallery_viewmodel.h"
#include "views/base_view.h"

namespace view {

class GalleryView : public BaseView {
 public:
  explicit GalleryView(lv_obj_t* parent);
  ~GalleryView() override;

  void bind(lv_subject_t* source_path_subject,
            lv_subject_t* preview_path_subject,
            lv_subject_t* counter_subject,
            lv_subject_t* title_subject,
            lv_subject_t* status_subject,
            lv_subject_t* empty_visible_subject,
            lv_subject_t* confirm_delete_subject,
            lv_subject_t* delete_choice_subject,
            lv_subject_t* info_visible_subject,
            lv_subject_t* info_text_subject,
            lv_subject_t* info_scroll_subject,
            lv_subject_t* detail_mode_subject,
            lv_subject_t* zoom_percent_subject,
            lv_subject_t* pan_x_subject,
            lv_subject_t* pan_y_subject);
  void set_observed_source_path(const char* path);
  void set_observed_preview_path(const char* path);
  void update_detail_mode_(bool enabled);
  void update_zoom_percent_(int32_t percent);
  void update_pan_x_(int32_t x);
  void update_pan_y_(int32_t y);
  void update_delete_choice_(int32_t choice);
  void update_info_scroll_(int32_t delta);
  void show_controls_temporarily();
  void hide_controls();

 private:
  void build_();
  void build_preview_();
  void build_top_bar_();
  void build_bottom_bar_();
  void build_detail_navigator_();
  void build_delete_dialog_();
  void build_info_overlay_();
  lv_obj_t* build_dialog_action_(lv_obj_t* parent, const char* text, const char* action_icon);
  void build_key_action_(const char* key_icon,
                         lv_font_t* key_font,
                         const char* action_icon,
                         lv_align_t align,
                         int32_t x_offset,
                         size_t action_index);
  void reload_image_();
  void apply_image_transform_();
  void update_bottom_actions_();
  void update_detail_navigator_();
  void set_preview_error_visible_(bool visible);
  lv_obj_t* preview_container_{nullptr};
  lv_obj_t* preview_image_{nullptr};
  lv_obj_t* preview_error_label_{nullptr};
  lv_obj_t* top_bar_{nullptr};
  lv_obj_t* counter_label_{nullptr};
  lv_obj_t* filename_label_{nullptr};
  lv_obj_t* empty_label_{nullptr};
  lv_obj_t* bottom_bar_{nullptr};
  std::array<lv_obj_t*, 5> bottom_action_labels_{};
  lv_obj_t* detail_navigator_{nullptr};
  lv_obj_t* detail_viewport_{nullptr};
  lv_obj_t* detail_zoom_label_{nullptr};
  lv_obj_t* dialog_scrim_{nullptr};
  lv_obj_t* dialog_{nullptr};
  lv_obj_t* dialog_title_label_{nullptr};
  lv_obj_t* dialog_body_label_{nullptr};
  lv_obj_t* dialog_cancel_btn_{nullptr};
  lv_obj_t* dialog_confirm_btn_{nullptr};
  lv_obj_t* info_scrim_{nullptr};
  lv_obj_t* info_panel_{nullptr};
  lv_obj_t* info_panel_title_{nullptr};
  lv_obj_t* info_panel_content_{nullptr};
  lv_obj_t* info_body_label_{nullptr};
  lv_obj_t* info_hint_label_{nullptr};

  std::string source_path_;
  std::string preview_path_;
  std::string lvgl_image_path_;
  ui::GalleryDrawBufferPtr decoded_image_;
  uint32_t image_width_{0};
  uint32_t image_height_{0};
  uint32_t fit_scale_{LV_SCALE_NONE};
  int32_t zoom_percent_{100};
  int32_t pan_x_{0};
  int32_t pan_y_{0};
  bool detail_mode_{false};
  bool controls_visible_{false};
  lv_timer_t* controls_hide_timer_{nullptr};
};

}  // namespace view

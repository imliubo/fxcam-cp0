#include "views/gallery_view.h"

#include <src/core/lv_obj.h>
#include <src/core/lv_obj_pos.h>
#include <src/core/lv_obj_scroll.h>
#include <src/core/lv_obj_style.h>
#include <src/core/lv_obj_style_gen.h>
#include <src/core/lv_observer.h>
#include <src/draw/lv_image_decoder.h>
#include <src/layouts/flex/lv_flex.h>
#include <src/misc/lv_area.h>
#include <src/misc/lv_color.h>
#include <src/misc/lv_timer.h>
#include <src/widgets/image/lv_image.h>
#include <src/widgets/label/lv_label.h>

#include <algorithm>
#include <cctype>
#include <filesystem>

#include "ui/app_color.h"
#include "ui/app_font.h"

namespace view {

using Font      = ui::font::AppFont;
namespace color = ui::color;
namespace font  = ui::font;

namespace {

constexpr int32_t kScreenWidth        = 320;
constexpr int32_t kScreenHeight       = 170;
constexpr int32_t kBottomBarHeight    = 40;
constexpr int32_t kActionLiftY        = -6;
constexpr int32_t kGuideLineHeight    = 6;
constexpr int32_t kInfoScrollStep     = 26;
constexpr int32_t kInfoPanelWidth     = 244;
constexpr int32_t kInfoPanelHeight    = 165;
constexpr int32_t kInfoTitleHeight    = 28;
constexpr int32_t kNavigatorWidth     = 72;
constexpr int32_t kNavigatorHeight    = 48;
constexpr int32_t kNavigatorPadding   = 4;
constexpr uint32_t kMaximumImageScale = 2048;
constexpr uint32_t kControlsVisibleMs = 3000;

void controls_hide_timer_cb(lv_timer_t* timer) {
  auto* view = static_cast<GalleryView*>(lv_timer_get_user_data(timer));
  if (view) {
    view->hide_controls();
  }
}

void style_transparent_container(lv_obj_t* obj) {
  lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(obj, 0, 0);
  lv_obj_set_style_radius(obj, 0, 0);
  lv_obj_set_style_pad_all(obj, 0, 0);
  lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

bool is_jpeg_path(std::string path) {
  std::transform(path.begin(), path.end(), path.begin(), [](unsigned char value) {
    return static_cast<char>(std::tolower(value));
  });
  const std::string extension = std::filesystem::path(path).extension().string();
  return extension == ".jpg" || extension == ".jpeg";
}

void gallery_source_path_observer_cb(lv_observer_t* observer, lv_subject_t* subject) {
  auto* view = static_cast<GalleryView*>(lv_observer_get_user_data(observer));
  if (!view) {
    return;
  }

  const char* path = lv_subject_get_string(subject);
  view->set_observed_source_path(path ? path : "");
}

void gallery_preview_path_observer_cb(lv_observer_t* observer, lv_subject_t* subject) {
  auto* view = static_cast<GalleryView*>(lv_observer_get_user_data(observer));
  if (!view) {
    return;
  }

  const char* path = lv_subject_get_string(subject);
  view->set_observed_preview_path(path ? path : "");
}

void gallery_detail_mode_observer_cb(lv_observer_t* observer, lv_subject_t* subject) {
  auto* view = static_cast<GalleryView*>(lv_observer_get_user_data(observer));
  if (view) {
    view->update_detail_mode_(lv_subject_get_int(subject) != 0);
  }
}

void gallery_zoom_observer_cb(lv_observer_t* observer, lv_subject_t* subject) {
  auto* view = static_cast<GalleryView*>(lv_observer_get_user_data(observer));
  if (view) {
    view->update_zoom_percent_(lv_subject_get_int(subject));
  }
}

void gallery_pan_x_observer_cb(lv_observer_t* observer, lv_subject_t* subject) {
  auto* view = static_cast<GalleryView*>(lv_observer_get_user_data(observer));
  if (view) {
    view->update_pan_x_(lv_subject_get_int(subject));
  }
}

void gallery_pan_y_observer_cb(lv_observer_t* observer, lv_subject_t* subject) {
  auto* view = static_cast<GalleryView*>(lv_observer_get_user_data(observer));
  if (view) {
    view->update_pan_y_(lv_subject_get_int(subject));
  }
}

void gallery_delete_choice_observer_cb(lv_observer_t* observer, lv_subject_t* subject) {
  auto* view = static_cast<GalleryView*>(lv_observer_get_user_data(observer));
  if (!view) {
    return;
  }

  view->update_delete_choice_(lv_subject_get_int(subject));
}

void gallery_info_text_observer_cb(lv_observer_t* observer, lv_subject_t* subject) {
  auto* view  = static_cast<GalleryView*>(lv_observer_get_user_data(observer));
  auto* label = lv_observer_get_target_obj(observer);
  if (!view || !label) {
    return;
  }

  const char* text = lv_subject_get_string(subject);
  lv_label_set_text(label, text ? text : "");
  view->update_info_scroll_(0);
}

void gallery_info_visible_observer_cb(lv_observer_t* observer, lv_subject_t* subject) {
  auto* view  = static_cast<GalleryView*>(lv_observer_get_user_data(observer));
  auto* scrim = lv_observer_get_target_obj(observer);
  if (!view || !scrim) {
    return;
  }

  if (lv_subject_get_int(subject) != 0) {
    lv_obj_remove_flag(scrim, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(scrim);
    view->update_info_scroll_(0);
  } else {
    lv_obj_add_flag(scrim, LV_OBJ_FLAG_HIDDEN);
  }
}

void gallery_info_scroll_observer_cb(lv_observer_t* observer, lv_subject_t* subject) {
  auto* view = static_cast<GalleryView*>(lv_observer_get_user_data(observer));
  if (!view) {
    return;
  }

  view->update_info_scroll_(lv_subject_get_int(subject) - lv_subject_get_previous_int(subject));
}

void style_dialog_action(lv_obj_t* obj, bool selected, bool destructive) {
  if (!obj) {
    return;
  }

  const uint32_t bg =
      selected ? (destructive ? color::DARK_ERRORCONTAINER : color::DARK_PRIMARYCONTAINER)
               : color::DARK_SURFACECONTAINER;
  const uint32_t border = selected ? (destructive ? color::DARK_ERROR : color::DARK_PRIMARY)
                                   : color::DARK_OUTLINEVARIANT;
  const uint32_t text =
      selected ? (destructive ? color::DARK_ONERRORCONTAINER : color::DARK_ONPRIMARYCONTAINER)
               : color::DARK_ONSURFACEVARIANT;

  lv_obj_set_style_bg_color(obj, lv_color_hex(bg), 0);
  lv_obj_set_style_bg_opa(obj, selected ? LV_OPA_COVER : LV_OPA_60, 0);
  lv_obj_set_style_border_color(obj, lv_color_hex(border), 0);
  lv_obj_set_style_border_width(obj, selected ? 2 : 1, 0);
  lv_obj_set_style_text_color(obj, lv_color_hex(text), 0);
}

}  // namespace

GalleryView::GalleryView(lv_obj_t* parent)
    : BaseView(parent) {
  build_();
  show_controls_temporarily();
}

GalleryView::~GalleryView() {
  if (controls_hide_timer_) {
    lv_timer_delete(controls_hide_timer_);
    controls_hide_timer_ = nullptr;
  }
  if (preview_image_) {
    lv_image_set_src(preview_image_, nullptr);
  }
  destroy();
  preview_image_ = nullptr;
  decoded_image_.reset();
}

void GalleryView::bind(lv_subject_t* source_path_subject,
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
                       lv_subject_t* pan_y_subject) {
  if (preview_image_ && source_path_subject) {
    lv_subject_add_observer_obj(source_path_subject,
                                gallery_source_path_observer_cb,
                                preview_image_,
                                this);
  }

  if (preview_image_ && preview_path_subject) {
    lv_subject_add_observer_obj(preview_path_subject,
                                gallery_preview_path_observer_cb,
                                preview_image_,
                                this);
  }

  if (counter_label_ && counter_subject) {
    lv_label_bind_text(counter_label_, counter_subject, nullptr);
  }

  if (filename_label_ && title_subject) {
    lv_label_bind_text(filename_label_, title_subject, nullptr);
  }

  if (empty_label_ && status_subject) {
    lv_label_bind_text(empty_label_, status_subject, nullptr);
  }

  if (empty_label_ && empty_visible_subject) {
    lv_obj_bind_flag_if_eq(empty_label_, empty_visible_subject, LV_OBJ_FLAG_HIDDEN, 0);
  }

  if (dialog_scrim_ && confirm_delete_subject) {
    lv_obj_bind_flag_if_eq(dialog_scrim_, confirm_delete_subject, LV_OBJ_FLAG_HIDDEN, 0);
  }

  if (dialog_ && delete_choice_subject) {
    lv_subject_add_observer_obj(delete_choice_subject,
                                gallery_delete_choice_observer_cb,
                                dialog_,
                                this);
    update_delete_choice_(lv_subject_get_int(delete_choice_subject));
  }

  if (info_scrim_ && info_visible_subject) {
    lv_subject_add_observer_obj(info_visible_subject,
                                gallery_info_visible_observer_cb,
                                info_scrim_,
                                this);
  }

  if (info_body_label_ && info_text_subject) {
    lv_subject_add_observer_obj(info_text_subject,
                                gallery_info_text_observer_cb,
                                info_body_label_,
                                this);
  }

  if (info_scrim_ && info_scroll_subject) {
    lv_subject_add_observer_obj(info_scroll_subject,
                                gallery_info_scroll_observer_cb,
                                info_scrim_,
                                this);
  }

  if (preview_image_ && detail_mode_subject) {
    lv_subject_add_observer_obj(detail_mode_subject,
                                gallery_detail_mode_observer_cb,
                                preview_image_,
                                this);
  }

  if (preview_image_ && zoom_percent_subject) {
    lv_subject_add_observer_obj(zoom_percent_subject,
                                gallery_zoom_observer_cb,
                                preview_image_,
                                this);
  }

  if (preview_image_ && pan_x_subject) {
    lv_subject_add_observer_obj(pan_x_subject, gallery_pan_x_observer_cb, preview_image_, this);
  }

  if (preview_image_ && pan_y_subject) {
    lv_subject_add_observer_obj(pan_y_subject, gallery_pan_y_observer_cb, preview_image_, this);
  }
}

void GalleryView::build_() {
  if (!root_) {
    return;
  }

  lv_obj_set_size(root_, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_color(root_, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(root_, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(root_, 0, 0);
  lv_obj_set_style_radius(root_, 0, 0);
  lv_obj_set_style_pad_all(root_, 0, 0);
  lv_obj_clear_flag(root_, LV_OBJ_FLAG_SCROLLABLE);

  build_preview_();
  build_top_bar_();
  build_bottom_bar_();
  build_detail_navigator_();
  build_delete_dialog_();
  build_info_overlay_();
}

void GalleryView::build_preview_() {
  preview_container_ = lv_obj_create(root_);
  lv_obj_set_size(preview_container_, LV_PCT(100), LV_PCT(100));
  style_transparent_container(preview_container_);

  preview_image_ = lv_image_create(preview_container_);
  lv_obj_set_size(preview_image_, 1, 1);
  lv_image_set_antialias(preview_image_, true);
  lv_obj_center(preview_image_);

  empty_label_ = lv_label_create(preview_container_);
  lv_obj_set_style_text_font(empty_label_, Font::inter_medium(16), 0);
  lv_obj_set_style_text_color(empty_label_, lv_color_hex(color::DARK_ONSURFACE), 0);
  lv_label_set_text(empty_label_, "No photos");
  lv_obj_center(empty_label_);

  preview_error_label_ = lv_label_create(preview_container_);
  lv_obj_set_style_text_font(preview_error_label_, Font::inter_medium(14), 0);
  lv_obj_set_style_text_color(preview_error_label_, lv_color_hex(color::DARK_ONSURFACEVARIANT), 0);
  lv_label_set_text(preview_error_label_, "No preview");
  lv_obj_center(preview_error_label_);
  lv_obj_add_flag(preview_error_label_, LV_OBJ_FLAG_HIDDEN);
}

void GalleryView::build_top_bar_() {
  top_bar_ = lv_obj_create(root_);
  lv_obj_set_size(top_bar_, LV_PCT(100), 34);
  style_transparent_container(top_bar_);
  lv_obj_set_style_bg_color(top_bar_, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(top_bar_, LV_OPA_50, 0);
  lv_obj_align(top_bar_, LV_ALIGN_TOP_MID, 0, 0);

  counter_label_ = lv_label_create(top_bar_);
  lv_obj_set_style_text_font(counter_label_, Font::inter_bold(13), 0);
  lv_obj_set_style_text_color(counter_label_, lv_color_white(), 0);
  lv_label_set_text(counter_label_, "0 / 0");
  lv_obj_align(counter_label_, LV_ALIGN_LEFT_MID, 8, 0);

  filename_label_ = lv_label_create(top_bar_);
  lv_obj_set_style_text_font(filename_label_, Font::inter_regular(12), 0);
  lv_obj_set_style_text_color(filename_label_, lv_color_hex(color::DARK_ONSURFACEVARIANT), 0);
  lv_obj_set_width(filename_label_, 210);
  lv_label_set_long_mode(filename_label_, LV_LABEL_LONG_DOT);
  lv_label_set_text(filename_label_, "Gallery");
  lv_obj_align(filename_label_, LV_ALIGN_RIGHT_MID, -8, 0);
}

void GalleryView::build_bottom_bar_() {
  bottom_bar_ = lv_obj_create(root_);
  lv_obj_set_size(bottom_bar_, LV_PCT(100), kBottomBarHeight);
  style_transparent_container(bottom_bar_);
  lv_obj_set_style_pad_gap(bottom_bar_, 10, 0);
  lv_obj_set_style_bg_color(bottom_bar_, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(bottom_bar_, LV_OPA_40, 0);
  lv_obj_align(bottom_bar_, LV_ALIGN_BOTTOM_MID, 0, 0);

  /* return key */
  build_key_action_(font::ICON_ARROW_U_UP_LEFT,
                    Font::camera_icons(32),
                    nullptr,
                    LV_ALIGN_LEFT_MID,
                    32,
                    0);

  /* left navigation key */
  build_key_action_(font::ICON_ARROW_LEFT,
                    Font::camera_icons(32),
                    nullptr,
                    LV_ALIGN_CENTER,
                    -56,
                    1);

  /* info icon key */
  build_key_action_(font::ICON_INFO, Font::camera_icons(32), nullptr, LV_ALIGN_CENTER, -3, 2);

  /* right navigation key */
  build_key_action_(font::ICON_ARROW_RIGHT,
                    Font::camera_icons(32),
                    nullptr,
                    LV_ALIGN_CENTER,
                    56,
                    3);

  /* delete key */
  build_key_action_(font::ICON_TRASH, Font::camera_icons(32), nullptr, LV_ALIGN_RIGHT_MID, -32, 4);
}

void GalleryView::build_detail_navigator_() {
  detail_navigator_ = lv_obj_create(root_);
  lv_obj_set_size(detail_navigator_, kNavigatorWidth, kNavigatorHeight);
  lv_obj_set_style_bg_color(detail_navigator_, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(detail_navigator_, LV_OPA_50, 0);
  lv_obj_set_style_border_color(detail_navigator_, lv_color_white(), 0);
  lv_obj_set_style_border_opa(detail_navigator_, LV_OPA_50, 0);
  lv_obj_set_style_border_width(detail_navigator_, 1, 0);
  lv_obj_set_style_radius(detail_navigator_, 2, 0);
  lv_obj_set_style_pad_all(detail_navigator_, kNavigatorPadding, 0);
  lv_obj_clear_flag(detail_navigator_, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align(detail_navigator_, LV_ALIGN_TOP_RIGHT, -8, 8);

  detail_viewport_ = lv_obj_create(detail_navigator_);
  lv_obj_set_size(detail_viewport_, kNavigatorWidth - kNavigatorPadding * 2, 22);
  lv_obj_set_style_bg_color(detail_viewport_, lv_color_white(), 0);
  lv_obj_set_style_bg_opa(detail_viewport_, LV_OPA_20, 0);
  lv_obj_set_style_border_color(detail_viewport_, lv_color_white(), 0);
  lv_obj_set_style_border_opa(detail_viewport_, LV_OPA_90, 0);
  lv_obj_set_style_border_width(detail_viewport_, 1, 0);
  lv_obj_set_style_radius(detail_viewport_, 0, 0);
  lv_obj_set_style_pad_all(detail_viewport_, 0, 0);
  lv_obj_clear_flag(detail_viewport_, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align(detail_viewport_, LV_ALIGN_TOP_LEFT, 0, 0);

  detail_zoom_label_ = lv_label_create(detail_navigator_);
  lv_obj_set_style_text_font(detail_zoom_label_, Font::inter_semibold(10), 0);
  lv_obj_set_style_text_color(detail_zoom_label_, lv_color_white(), 0);
  lv_label_set_text(detail_zoom_label_, "100%");
  lv_obj_align(detail_zoom_label_, LV_ALIGN_BOTTOM_RIGHT, 0, 1);

  lv_obj_add_flag(detail_navigator_, LV_OBJ_FLAG_HIDDEN);
}

void GalleryView::build_delete_dialog_() {
  dialog_scrim_ = lv_obj_create(root_);
  lv_obj_set_size(dialog_scrim_, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_color(dialog_scrim_, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(dialog_scrim_, LV_OPA_70, 0);
  lv_obj_set_style_border_width(dialog_scrim_, 0, 0);
  lv_obj_set_style_radius(dialog_scrim_, 0, 0);
  lv_obj_set_style_pad_all(dialog_scrim_, 0, 0);
  lv_obj_clear_flag(dialog_scrim_, LV_OBJ_FLAG_SCROLLABLE);

  dialog_ = lv_obj_create(dialog_scrim_);
  lv_obj_set_size(dialog_, 236, 112);
  lv_obj_set_style_bg_color(dialog_, lv_color_hex(color::DARK_SURFACECONTAINERHIGH), 0);
  lv_obj_set_style_bg_opa(dialog_, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(dialog_, lv_color_hex(color::DARK_OUTLINEVARIANT), 0);
  lv_obj_set_style_border_width(dialog_, 1, 0);
  lv_obj_set_style_radius(dialog_, 8, 0);
  lv_obj_set_style_pad_all(dialog_, 10, 0);
  lv_obj_clear_flag(dialog_, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_center(dialog_);

  dialog_title_label_ = lv_label_create(dialog_);
  lv_obj_set_style_text_font(dialog_title_label_, Font::inter_bold(15), 0);
  lv_obj_set_style_text_color(dialog_title_label_, lv_color_white(), 0);
  lv_label_set_text(dialog_title_label_, "Delete photo?");
  lv_obj_align(dialog_title_label_, LV_ALIGN_TOP_LEFT, 0, 0);

  dialog_body_label_ = lv_label_create(dialog_);
  lv_obj_set_style_text_font(dialog_body_label_, Font::inter_regular(12), 0);
  lv_obj_set_style_text_color(dialog_body_label_, lv_color_hex(color::DARK_ONSURFACEVARIANT), 0);
  lv_obj_set_width(dialog_body_label_, 216);
  lv_label_set_text(dialog_body_label_, "This cannot be undone.");
  lv_obj_align(dialog_body_label_, LV_ALIGN_TOP_LEFT, 0, 23);

  lv_obj_t* actions = lv_obj_create(dialog_);
  lv_obj_set_size(actions, 216, 34);
  style_transparent_container(actions);
  lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(actions,
                        LV_FLEX_ALIGN_SPACE_BETWEEN,
                        LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_align(actions, LV_ALIGN_BOTTOM_MID, 0, 0);

  dialog_cancel_btn_  = build_dialog_action_(actions, "Cancel", font::ICON_CROSS);
  dialog_confirm_btn_ = build_dialog_action_(actions, "Confirm", font::ICON_CHECK);
  update_delete_choice_(0);

  lv_obj_add_flag(dialog_scrim_, LV_OBJ_FLAG_HIDDEN);
}

void GalleryView::build_info_overlay_() {
  info_scrim_ = lv_obj_create(root_);
  lv_obj_set_size(info_scrim_, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_color(info_scrim_, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(info_scrim_, LV_OPA_70, 0);
  lv_obj_set_style_border_width(info_scrim_, 0, 0);
  lv_obj_set_style_radius(info_scrim_, 0, 0);
  lv_obj_set_style_pad_top(info_scrim_, 10, 0);
  lv_obj_set_style_pad_bottom(info_scrim_, 10, 0);
  lv_obj_set_style_pad_left(info_scrim_, 0, 0);
  lv_obj_set_style_pad_right(info_scrim_, 0, 0);
  lv_obj_clear_flag(info_scrim_, LV_OBJ_FLAG_SCROLLABLE);

  info_panel_ = lv_obj_create(info_scrim_);
  lv_obj_set_size(info_panel_, kInfoPanelWidth, kInfoPanelHeight);
  lv_obj_set_style_bg_color(info_panel_, lv_color_hex(color::DARK_SURFACECONTAINERHIGH), 0);
  lv_obj_set_style_bg_opa(info_panel_, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(info_panel_, lv_color_hex(color::DARK_OUTLINEVARIANT), 0);
  lv_obj_set_style_border_width(info_panel_, 1, 0);
  lv_obj_set_style_radius(info_panel_, 8, 0);
  lv_obj_set_style_pad_all(info_panel_, 10, 0);
  lv_obj_clear_flag(info_panel_, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align(info_panel_, LV_ALIGN_TOP_MID, 0, 0);

  info_panel_title_ = lv_obj_create(info_panel_);
  lv_obj_set_size(info_panel_title_, LV_PCT(100), kInfoTitleHeight);
  style_transparent_container(info_panel_title_);
  lv_obj_align(info_panel_title_, LV_ALIGN_TOP_MID, 0, 0);

  lv_obj_t* title = lv_label_create(info_panel_title_);
  lv_obj_set_style_text_font(title, Font::inter_bold(15), 0);
  lv_obj_set_style_text_color(title, lv_color_white(), 0);
  lv_label_set_text(title, "Photo info");
  lv_obj_align(title, LV_ALIGN_TOP_LEFT, 0, 0);

  info_hint_label_ = lv_label_create(info_panel_title_);
  lv_obj_set_style_text_font(info_hint_label_, Font::inter_semibold(10), 0);
  lv_obj_set_style_text_color(info_hint_label_, lv_color_hex(color::DARK_ONSURFACEVARIANT), 0);
  lv_label_set_text(info_hint_label_, "ESC back");
  lv_obj_align(info_hint_label_, LV_ALIGN_TOP_RIGHT, 0, 2);

  info_panel_content_ = lv_obj_create(info_panel_);
  lv_obj_set_size(info_panel_content_, LV_PCT(100), kInfoPanelHeight - kInfoTitleHeight - 20);
  style_transparent_container(info_panel_content_);
  lv_obj_add_flag(info_panel_content_, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(
      info_panel_content_,
      static_cast<lv_obj_flag_t>(LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM |
                                 LV_OBJ_FLAG_SCROLL_CHAIN));
  lv_obj_set_scroll_dir(info_panel_content_, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(info_panel_content_, LV_SCROLLBAR_MODE_AUTO);
  lv_obj_align(info_panel_content_, LV_ALIGN_BOTTOM_MID, 0, 0);

  info_body_label_ = lv_label_create(info_panel_content_);
  lv_obj_set_style_text_font(info_body_label_, Font::inter_regular(11), 0);
  lv_obj_set_style_text_color(info_body_label_, lv_color_hex(color::DARK_ONSURFACEVARIANT), 0);
  lv_obj_set_width(info_body_label_, 224);
  lv_label_set_long_mode(info_body_label_, LV_LABEL_LONG_WRAP);
  lv_label_set_text(info_body_label_, "");
  lv_obj_align(info_body_label_, LV_ALIGN_TOP_LEFT, 0, 0);

  lv_obj_add_flag(info_scrim_, LV_OBJ_FLAG_HIDDEN);
}

lv_obj_t* GalleryView::build_dialog_action_(lv_obj_t* parent,
                                            const char* text,
                                            const char* action_icon) {
  lv_obj_t* action = lv_obj_create(parent);
  lv_obj_set_size(action, 76, 30);
  lv_obj_set_style_radius(action, 6, 0);
  lv_obj_set_style_pad_hor(action, 6, 0);
  lv_obj_set_style_pad_ver(action, 0, 0);
  lv_obj_set_style_pad_column(action, 4, 0);
  lv_obj_clear_flag(action, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_flex_flow(action, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(action, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  lv_obj_t* action_label = lv_label_create(action);
  lv_obj_set_style_text_font(action_label, Font::camera_icons(16), 0);
  lv_label_set_text(action_label, action_icon ? action_icon : "");
  lv_obj_set_style_translate_y(action_label, -1, 0);

  lv_obj_t* text_label = lv_label_create(action);
  lv_obj_set_style_text_font(text_label, Font::inter_semibold(11), 0);
  lv_label_set_text(text_label, text ? text : "");

  return action;
}

void GalleryView::build_key_action_(const char* key_icon,
                                    lv_font_t* key_font,
                                    const char* action_icon,
                                    lv_align_t align,
                                    int32_t x_offset,
                                    size_t action_index) {
  lv_obj_t* key = lv_obj_create(bottom_bar_);
  lv_obj_set_size(key, 36, 36);
  style_transparent_container(key);
  lv_obj_align(key, align, x_offset, kActionLiftY);

  lv_obj_t* key_label = lv_label_create(key);
  lv_obj_set_style_text_font(key_label, key_font, 0);
  lv_obj_set_style_text_color(key_label, lv_color_white(), 0);
  lv_label_set_text(key_label, key_icon ? key_icon : "");
  lv_obj_align(key_label, LV_ALIGN_CENTER, action_icon ? -7 : 0, 0);
  if (action_index < bottom_action_labels_.size()) {
    bottom_action_labels_[action_index] = key_label;
  }

  if (action_icon) {
    lv_obj_t* action_label = lv_label_create(key);
    lv_obj_set_style_text_font(action_label, Font::camera_icons(32), 0);
    lv_obj_set_style_text_color(action_label, lv_color_hex(color::DARK_ERROR), 0);
    lv_label_set_text(action_label, action_icon);
    lv_obj_align(action_label, LV_ALIGN_CENTER, 11, 0);
  }

  lv_obj_t* guide_line = lv_obj_create(bottom_bar_);
  lv_obj_set_size(guide_line, 2, kGuideLineHeight);
  lv_obj_set_style_bg_color(guide_line, lv_color_white(), 0);
  lv_obj_set_style_bg_opa(guide_line, LV_OPA_60, 0);
  lv_obj_set_style_border_width(guide_line, 0, 0);
  lv_obj_set_style_radius(guide_line, 1, 0);
  lv_obj_set_style_pad_all(guide_line, 0, 0);
  lv_obj_clear_flag(guide_line, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align_to(guide_line, key, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
}

void GalleryView::set_observed_source_path(const char* path) {
  source_path_ = path ? path : "";
  if (detail_mode_) {
    reload_image_();
  }
}

void GalleryView::set_observed_preview_path(const char* path) {
  preview_path_ = path ? path : "";
  if (!detail_mode_) {
    reload_image_();
  }
}

void GalleryView::update_detail_mode_(bool enabled) {
  if (detail_mode_ == enabled) {
    return;
  }

  detail_mode_ = enabled;
  if (top_bar_) {
    if (detail_mode_) {
      lv_obj_add_flag(top_bar_, LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_remove_flag(top_bar_, LV_OBJ_FLAG_HIDDEN);
    }
  }
  update_bottom_actions_();
  reload_image_();
}

void GalleryView::update_zoom_percent_(int32_t percent) {
  zoom_percent_ = std::clamp(percent, 100, 800);
  apply_image_transform_();
}

void GalleryView::update_pan_x_(int32_t x) {
  pan_x_ = std::clamp(x, -100, 100);
  apply_image_transform_();
}

void GalleryView::update_pan_y_(int32_t y) {
  pan_y_ = std::clamp(y, -100, 100);
  apply_image_transform_();
}

void GalleryView::show_controls_temporarily() {
  controls_visible_ = true;
  if (bottom_bar_) {
    lv_obj_remove_flag(bottom_bar_, LV_OBJ_FLAG_HIDDEN);
  }
  update_detail_navigator_();

  if (controls_hide_timer_) {
    lv_timer_delete(controls_hide_timer_);
  }
  controls_hide_timer_ = lv_timer_create(controls_hide_timer_cb, kControlsVisibleMs, this);
  if (controls_hide_timer_) {
    lv_timer_set_repeat_count(controls_hide_timer_, 1);
  }
}

void GalleryView::hide_controls() {
  controls_hide_timer_ = nullptr;
  controls_visible_    = false;
  if (bottom_bar_) {
    lv_obj_add_flag(bottom_bar_, LV_OBJ_FLAG_HIDDEN);
  }
  if (detail_navigator_) {
    lv_obj_add_flag(detail_navigator_, LV_OBJ_FLAG_HIDDEN);
  }
}

void GalleryView::reload_image_() {
  if (!preview_image_) {
    return;
  }

  lv_image_set_src(preview_image_, nullptr);
  decoded_image_.reset();
  lvgl_image_path_.clear();
  image_width_  = 0;
  image_height_ = 0;

  const std::string& path = detail_mode_ ? source_path_ : preview_path_;
  if (path.empty()) {
    set_preview_error_visible_(false);
    lv_obj_set_size(preview_image_, 1, 1);
    update_detail_navigator_();
    return;
  }

  if (detail_mode_ && is_jpeg_path(path)) {
    decoded_image_ = ui::decode_gallery_jpeg(path);
    if (decoded_image_) {
      image_width_  = decoded_image_->header.w;
      image_height_ = decoded_image_->header.h;
      lv_image_set_src(preview_image_, decoded_image_.get());
    }
  } else {
    lvgl_image_path_ = std::string("A:") + path;
    lv_image_header_t header{};
    if (lv_image_decoder_get_info(lvgl_image_path_.c_str(), &header) == LV_RESULT_OK) {
      image_width_  = header.w;
      image_height_ = header.h;
      lv_image_set_src(preview_image_, lvgl_image_path_.c_str());
    }
  }

  if (image_width_ == 0 || image_height_ == 0) {
    set_preview_error_visible_(true);
    lv_obj_set_size(preview_image_, 1, 1);
    update_detail_navigator_();
    return;
  }

  set_preview_error_visible_(false);
  lv_obj_set_size(preview_image_,
                  static_cast<int32_t>(image_width_),
                  static_cast<int32_t>(image_height_));
  lv_image_set_pivot(preview_image_,
                     static_cast<int32_t>(image_width_ / 2),
                     static_cast<int32_t>(image_height_ / 2));
  lv_image_set_antialias(preview_image_, true);
  apply_image_transform_();
}

void GalleryView::apply_image_transform_() {
  if (!preview_image_ || image_width_ == 0 || image_height_ == 0) {
    update_detail_navigator_();
    return;
  }

  const int32_t viewport_height = kScreenHeight;
  const uint32_t scale_x =
      static_cast<uint32_t>(static_cast<uint64_t>(kScreenWidth) * LV_SCALE_NONE / image_width_);
  const uint32_t scale_y =
      static_cast<uint32_t>(static_cast<uint64_t>(viewport_height) * LV_SCALE_NONE / image_height_);
  fit_scale_ =
      std::max<uint32_t>(1, std::min({scale_x, scale_y, static_cast<uint32_t>(LV_SCALE_NONE)}));
  const uint32_t scale = std::clamp<uint32_t>(
      static_cast<uint32_t>(static_cast<uint64_t>(fit_scale_) * zoom_percent_ / 100),
      1,
      kMaximumImageScale);
  lv_image_set_scale(preview_image_, scale);

  const int32_t transformed_width =
      static_cast<int32_t>(static_cast<uint64_t>(image_width_) * scale / LV_SCALE_NONE);
  const int32_t transformed_height =
      static_cast<int32_t>(static_cast<uint64_t>(image_height_) * scale / LV_SCALE_NONE);
  const int32_t max_offset_x = std::max<int32_t>(0, (transformed_width - kScreenWidth) / 2);
  const int32_t max_offset_y = std::max<int32_t>(0, (transformed_height - viewport_height) / 2);
  const int32_t offset_x     = max_offset_x * pan_x_ / 100;
  const int32_t offset_y     = max_offset_y * pan_y_ / 100;
  const int32_t image_x      = kScreenWidth / 2 - static_cast<int32_t>(image_width_) / 2 - offset_x;
  const int32_t image_y = kScreenHeight / 2 - static_cast<int32_t>(image_height_) / 2 - offset_y;
  lv_obj_align(preview_image_, LV_ALIGN_TOP_LEFT, image_x, image_y);
  lv_obj_invalidate(preview_image_);
  update_detail_navigator_();
}

void GalleryView::update_bottom_actions_() {
  constexpr std::array<const char*, 5> kOverviewActions{{font::ICON_ARROW_U_UP_LEFT,
                                                         font::ICON_ARROW_LEFT,
                                                         font::ICON_INFO,
                                                         font::ICON_ARROW_RIGHT,
                                                         font::ICON_TRASH}};
  constexpr std::array<const char*, 5> kDetailActions{{font::ICON_ARROW_U_UP_LEFT,
                                                       font::ICON_ZOOM_OUT,
                                                       font::ICON_INFO,
                                                       font::ICON_ZOOM_IN,
                                                       font::ICON_GRID_FOUR}};
  const auto& actions = detail_mode_ ? kDetailActions : kOverviewActions;
  for (size_t index = 0; index < bottom_action_labels_.size(); ++index) {
    if (bottom_action_labels_[index]) {
      lv_label_set_text(bottom_action_labels_[index], actions[index]);
    }
  }
}

void GalleryView::update_detail_navigator_() {
  if (!detail_navigator_) {
    return;
  }
  if (!detail_mode_ || image_width_ == 0 || image_height_ == 0) {
    lv_obj_add_flag(detail_navigator_, LV_OBJ_FLAG_HIDDEN);
    return;
  }
  if (!controls_visible_) {
    lv_obj_add_flag(detail_navigator_, LV_OBJ_FLAG_HIDDEN);
    return;
  }

  lv_obj_remove_flag(detail_navigator_, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(detail_navigator_);
  if (detail_zoom_label_) {
    lv_label_set_text_fmt(detail_zoom_label_, "%d%%", zoom_percent_);
  }

  const uint32_t scale = std::clamp<uint32_t>(
      static_cast<uint32_t>(static_cast<uint64_t>(fit_scale_) * zoom_percent_ / 100),
      1,
      kMaximumImageScale);
  const int32_t transformed_width =
      static_cast<int32_t>(static_cast<uint64_t>(image_width_) * scale / LV_SCALE_NONE);
  const int32_t transformed_height =
      static_cast<int32_t>(static_cast<uint64_t>(image_height_) * scale / LV_SCALE_NONE);
  const int32_t inner_width  = kNavigatorWidth - kNavigatorPadding * 2;
  const int32_t inner_height = 24;
  const int32_t viewport_width =
      std::clamp<int32_t>(inner_width * kScreenWidth / std::max(kScreenWidth, transformed_width),
                          5,
                          inner_width);
  const int32_t viewport_height = std::clamp<int32_t>(
      inner_height * kScreenHeight / std::max(kScreenHeight, transformed_height),
      4,
      inner_height);
  const int32_t available_x = inner_width - viewport_width;
  const int32_t available_y = inner_height - viewport_height;
  const int32_t x =
      transformed_width <= kScreenWidth ? available_x / 2 : available_x * (pan_x_ + 100) / 200;
  const int32_t y =
      transformed_height <= kScreenHeight ? available_y / 2 : available_y * (pan_y_ + 100) / 200;
  if (detail_viewport_) {
    lv_obj_set_size(detail_viewport_, viewport_width, viewport_height);
    lv_obj_align(detail_viewport_, LV_ALIGN_TOP_LEFT, x, y);
  }
}

void GalleryView::set_preview_error_visible_(bool visible) {
  if (!preview_error_label_) {
    return;
  }
  if (visible) {
    lv_obj_remove_flag(preview_error_label_, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(preview_error_label_, LV_OBJ_FLAG_HIDDEN);
  }
}

void GalleryView::update_delete_choice_(int32_t choice) {
  style_dialog_action(dialog_cancel_btn_, choice == 0, false);
  style_dialog_action(dialog_confirm_btn_, choice != 0, true);
}

void GalleryView::update_info_scroll_(int32_t delta) {
  if (!info_panel_content_) {
    return;
  }

  if (delta == 0) {
    lv_obj_scroll_to_y(info_panel_content_, 0, LV_ANIM_OFF);
    return;
  }

  lv_obj_scroll_by_bounded(info_panel_content_, 0, -delta * kInfoScrollStep, LV_ANIM_ON);
}

}  // namespace view

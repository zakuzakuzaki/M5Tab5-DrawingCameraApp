#include "app_drawing_camera.h"
#include <hal/hal.h>
#include <mooncake_log.h>
#include <smooth_lvgl.h>

using namespace mooncake;

static void palette_event_cb(lv_event_t* e)
{
    auto app            = static_cast<AppDrawingCamera*>(lv_event_get_user_data(e));
    lv_obj_t* btn       = lv_event_get_target(e);
    app->_current_color = lv_obj_get_style_bg_color(btn, LV_PART_MAIN);
}

static void canvas_event_cb(lv_event_t* e)
{
    AppDrawingCamera* app = static_cast<AppDrawingCamera*>(lv_event_get_user_data(e));
    if (lv_event_get_code(e) == LV_EVENT_PRESSING && !app->_camera_mode) {
        lv_point_t p;
        lv_indev_get_point(lv_indev_get_act(), &p);
        lv_draw_rect_dsc_t dsc;
        lv_draw_rect_dsc_init(&dsc);
        dsc.bg_color     = app->_current_color;
        dsc.border_width = 0;
        lv_canvas_draw_rect(app->_canvas, p.x - 4, p.y - 4, 8, 8, &dsc);
    }
}

AppDrawingCamera::AppDrawingCamera()
{
    setAppInfo().name = "AppDrawingCamera";
}

void AppDrawingCamera::onCreate()
{
    open();
}

void AppDrawingCamera::onOpen()
{
    const uint16_t w = 1280;
    const uint16_t h = 720;
    _canvas_buf.resize(w * h);
    _canvas = lv_canvas_create(lv_screen_active());
    lv_canvas_set_buffer(_canvas, _canvas_buf.data(), w, h, LV_COLOR_FORMAT_RGB565);
    lv_obj_center(_canvas);
    lv_canvas_fill_bg(_canvas, lv_color_white(), LV_OPA_COVER);
    lv_obj_add_event_cb(_canvas, canvas_event_cb, LV_EVENT_ALL, this);

    /* palette */
    _palette = lv_obj_create(lv_screen_active());
    lv_obj_set_size(_palette, w, 60);
    lv_obj_align(_palette, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_flex_flow(_palette, LV_FLEX_FLOW_ROW);

    lv_color_t colors[4] = {lv_palette_main(LV_PALETTE_RED), lv_palette_main(LV_PALETTE_GREEN),
                            lv_palette_main(LV_PALETTE_BLUE), lv_color_black()};
    for (int i = 0; i < 4; i++) {
        lv_obj_t* btn = lv_btn_create(_palette);
        lv_obj_set_size(btn, 60, 40);
        lv_obj_set_style_bg_color(btn, colors[i], LV_PART_MAIN);
        lv_obj_add_event_cb(btn, palette_event_cb, LV_EVENT_CLICKED, this);
    }
    _current_color = colors[0];

    _camera_btn = lv_btn_create(lv_screen_active());
    lv_obj_align(_camera_btn, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_add_event_cb(
        _camera_btn,
        [](lv_event_t* e) {
            auto app = static_cast<AppDrawingCamera*>(lv_event_get_user_data(e));
            if (!app->_camera_mode) {
                GetHAL()->startCameraCapture(app->_canvas);
                app->_camera_mode = true;
                lv_obj_add_flag(app->_palette, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(app->_capture_btn, LV_OBJ_FLAG_HIDDEN);
            }
        },
        LV_EVENT_CLICKED, this);
    lv_obj_t* label = lv_label_create(_camera_btn);
    lv_label_set_text(label, "Camera");

    _capture_btn = lv_btn_create(lv_screen_active());
    lv_obj_align(_capture_btn, LV_ALIGN_TOP_RIGHT, -10, 70);
    lv_obj_add_flag(_capture_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(
        _capture_btn,
        [](lv_event_t* e) {
            auto app = static_cast<AppDrawingCamera*>(lv_event_get_user_data(e));
            if (app->_camera_mode) {
                /* copy frame */
                size_t sz = 1280 * 720 * sizeof(lv_color_t);
                memcpy(app->_canvas_buf.data(), lv_canvas_get_buffer(app->_canvas), sz);
                GetHAL()->stopCameraCapture();
                lv_canvas_set_buffer(app->_canvas, app->_canvas_buf.data(), 1280, 720, LV_COLOR_FORMAT_RGB565);
                app->_camera_mode = false;
                lv_obj_clear_flag(app->_palette, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(app->_capture_btn, LV_OBJ_FLAG_HIDDEN);
            }
        },
        LV_EVENT_CLICKED, this);
    lv_obj_t* label2 = lv_label_create(_capture_btn);
    lv_label_set_text(label2, "Capture");
}

void AppDrawingCamera::onRunning()
{
}

void AppDrawingCamera::onClose()
{
    if (_camera_mode) {
        GetHAL()->stopCameraCapture();
    }
    lv_obj_del(_canvas);
    lv_obj_del(_palette);
    lv_obj_del(_camera_btn);
    lv_obj_del(_capture_btn);
}

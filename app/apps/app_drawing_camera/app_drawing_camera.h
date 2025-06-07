#pragma once
#include <mooncake.h>
#include <lvgl.h>
#include <vector>

class AppDrawingCamera : public mooncake::AppAbility {
public:
    AppDrawingCamera();
    void onCreate() override;
    void onOpen() override;
    void onRunning() override;
    void onClose() override;

private:
    lv_obj_t* _canvas = nullptr;
    std::vector<lv_color_t> _canvas_buf;
    lv_obj_t* _palette        = nullptr;
    lv_obj_t* _camera_btn     = nullptr;
    lv_obj_t* _capture_btn    = nullptr;
    lv_color_t _current_color = lv_color_make(0, 0, 0);
    bool _camera_mode         = false;
};

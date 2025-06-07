/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "view.h"
#include <lvgl.h>
#include <hal/hal.h>
#include <mooncake.h>
#include <mooncake_log.h>
#include <smooth_ui_toolkit.h>
#include <smooth_lvgl.h>
#include <apps/utils/audio/audio.h>

using namespace launcher_view;
using namespace smooth_ui_toolkit;
using namespace smooth_ui_toolkit::lvgl_cpp;

static const std::string _tag = "panel-drawing-camera";

static const ui::Window::KeyFrame_t _kf_drawing_camera_close = {120, 450, 74, 74, 0};
static const ui::Window::KeyFrame_t _kf_drawing_camera_open  = {50, 50, 400, 300, 255};

class DrawingCameraWindow : public ui::Window {
public:
    DrawingCameraWindow()
    {
        config.kfClosed = _kf_drawing_camera_close;
        config.kfOpened = _kf_drawing_camera_open;
    }

    void onOpen() override
    {
        _window->setScrollbarMode(LV_SCROLLBAR_MODE_OFF);

        // タイトル
        _label_title = std::make_unique<Label>(_window->get());
        _label_title->align(LV_ALIGN_TOP_MID, 0, 20);
        _label_title->setTextFont(&lv_font_montserrat_24);
        _label_title->setText("Drawing Camera App");

        // 説明テキスト
        _label_desc = std::make_unique<Label>(_window->get());
        _label_desc->align(LV_ALIGN_CENTER, 0, -20);
        _label_desc->setTextFont(&lv_font_montserrat_16);
        _label_desc->setText("Touch drawing with camera\nbackground support");

        // 起動ボタン
        _btn_start = std::make_unique<Button>(_window->get());
        _btn_start->align(LV_ALIGN_BOTTOM_MID, 0, -30);
        _btn_start->setSize(120, 40);
        _btn_start->setBgColor(lv_color_hex(0x4CAF50));
        _btn_start->setRadius(10);
        _btn_start->onClick().connect([&]() {
            audio::play_next_tone_progression();
            
            // お絵描きアプリを起動
            auto app_count = mooncake::GetMooncake().getAppNum();
            for (int i = 0; i < app_count; i++) {
                auto app_info = mooncake::GetMooncake().getAppInfo(i);
                if (app_info.name == "DrawingCamera") {
                    mooncake::GetMooncake().openApp(i);
                    close();
                    break;
                }
            }
        });

        _btn_start->label().setTextColor(lv_color_white());
        _btn_start->label().setTextFont(&lv_font_montserrat_16);
        _btn_start->label().setText("Start App");
    }

    void onClose() override
    {
        audio::play_next_tone_progression();
    }

private:
    std::unique_ptr<Label> _label_title;
    std::unique_ptr<Label> _label_desc;
    std::unique_ptr<Button> _btn_start;
};

void PanelDrawingCamera::init()
{
    _btn_drawing_camera = std::make_unique<Container>(lv_screen_active());
    _btn_drawing_camera->align(LV_ALIGN_CENTER, 120, 450);
    _btn_drawing_camera->setSize(83, 97);
    _btn_drawing_camera->setOpa(0);
    _btn_drawing_camera->onClick().connect([&] {
        audio::play_next_tone_progression();

        // Create window
        _window = std::make_unique<DrawingCameraWindow>();
        _window->init(lv_screen_active());
        _window->open();
    });
}

void PanelDrawingCamera::update(bool isStacked)
{
    if (_window) {
        _window->update();
        if (_window->getState() == ui::Window::State_t::Closed) {
            _window.reset();
        }
    }
}
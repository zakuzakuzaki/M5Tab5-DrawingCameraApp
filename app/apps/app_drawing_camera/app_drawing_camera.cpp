/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "app_drawing_camera.h"
#include <hal/hal.h>
#include <mooncake.h>
#include <mooncake_log.h>
#include <cstring>  // memcpy用

using namespace mooncake;

AppDrawingCamera::AppDrawingCamera()
{
    setAppInfo().name = "DrawingCamera";
}

void AppDrawingCamera::onCreate()
{
    mclog::tagInfo(getAppInfo().name, "on create");
    
    // 描画画面とカメラ画面を初期化
    initDrawingScreen();
    initCameraScreen();
    
    open();
}

void AppDrawingCamera::onOpen()
{
    mclog::tagInfo(getAppInfo().name, "on open");
    
    // 描画モードで開始
    switchToDrawingMode();
}

void AppDrawingCamera::onRunning()
{
    // カメラモード中の状態更新
    if (_current_state == STATE_CAMERA_PREVIEW && _camera_info_label) {
        LvglLockGuard lock;
        
        static uint32_t last_update = 0;
        uint32_t now = GetHAL()->millis();
        
        // 1秒毎に状態を更新
        if (now - last_update > 1000) {
            bool is_capturing = GetHAL()->isCameraCapturing();
            if (is_capturing) {
                lv_label_set_text(_camera_info_label, "CAMERA MODE\n1280x720\nCAPTURING");
            } else {
                lv_label_set_text(_camera_info_label, "CAMERA MODE\n1280x720\nNOT CAPTURING");
            }
            last_update = now;
        }
    }
}

void AppDrawingCamera::onClose()
{
    mclog::tagInfo(getAppInfo().name, "on close");
    
    // カメラキャプチャを停止
    if (_current_state == STATE_CAMERA_PREVIEW) {
        GetHAL()->stopCameraCapture();
    }
    
    // バッファをクリーンアップ
    if (_canvas_buffer) {
        lv_draw_buf_destroy(_canvas_buffer);
        _canvas_buffer = nullptr;
    }
}

void AppDrawingCamera::initDrawingScreen()
{
    LvglLockGuard lock;
    
    // メイン画面作成
    _main_screen = lv_obj_create(nullptr);
    lv_obj_set_size(_main_screen, lv_display_get_horizontal_resolution(lv_display_get_default()), 
                                  lv_display_get_vertical_resolution(lv_display_get_default()));
    lv_obj_set_style_bg_color(_main_screen, lv_color_black(), 0);
    
    // キャンバス作成（全画面サイズ）
    _canvas = lv_canvas_create(_main_screen);
    lv_obj_set_size(_canvas, CANVAS_WIDTH, CANVAS_HEIGHT);
    lv_obj_align(_canvas, LV_ALIGN_CENTER, 0, 0);  // 画面中央に配置
    
    // キャンバス用バッファ作成
    _canvas_buffer = lv_draw_buf_create(CANVAS_WIDTH, CANVAS_HEIGHT, LV_COLOR_FORMAT_RGB565, LV_STRIDE_AUTO);
    lv_canvas_set_draw_buf(_canvas, _canvas_buffer);
    
    // キャンバスを白で初期化
    lv_canvas_fill_bg(_canvas, lv_color_white(), LV_OPA_COVER);
    
    // キャンバスのタッチイベント設定
    lv_obj_add_event_cb(_canvas, canvasEventHandler, LV_EVENT_PRESSED, this);
    lv_obj_add_event_cb(_canvas, canvasEventHandler, LV_EVENT_PRESSING, this);
    lv_obj_add_flag(_canvas, LV_OBJ_FLAG_CLICKABLE);
    
    // カラーパレット作成（キャンバスの上に重ねて表示）
    initColorPalette();
    
    // カメラボタン（キャンバスの上に重ねて表示）
    _camera_btn = lv_btn_create(_main_screen);
    lv_obj_set_size(_camera_btn, 80, 40);
    lv_obj_align(_camera_btn, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
    lv_obj_add_event_cb(_camera_btn, cameraBtnEventHandler, LV_EVENT_CLICKED, this);
    lv_obj_move_foreground(_camera_btn);  // 前面に移動
    
    lv_obj_t* camera_label = lv_label_create(_camera_btn);
    lv_label_set_text(camera_label, "Camera");
    lv_obj_center(camera_label);
    
    // 戻るボタン（キャンバスの上に重ねて表示）
    _back_btn = lv_btn_create(_main_screen);
    lv_obj_set_size(_back_btn, 60, 40);
    lv_obj_align(_back_btn, LV_ALIGN_BOTTOM_LEFT, 20, -20);
    lv_obj_add_event_cb(_back_btn, backBtnEventHandler, LV_EVENT_CLICKED, this);
    lv_obj_move_foreground(_back_btn);  // 前面に移動
    
    lv_obj_t* back_label = lv_label_create(_back_btn);
    lv_label_set_text(back_label, "Back");
    lv_obj_center(back_label);
    
    // クリアボタン（キャンバスの上に重ねて表示）
    _clear_btn = lv_btn_create(_main_screen);
    lv_obj_set_size(_clear_btn, 60, 40);
    lv_obj_align(_clear_btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_add_event_cb(_clear_btn, clearBtnEventHandler, LV_EVENT_CLICKED, this);
    lv_obj_move_foreground(_clear_btn);  // 前面に移動
    
    lv_obj_t* clear_label = lv_label_create(_clear_btn);
    lv_label_set_text(clear_label, "Clear");
    lv_obj_center(clear_label);
}

void AppDrawingCamera::initCameraScreen()
{
    LvglLockGuard lock;
    
    // カメラ画面作成
    _camera_screen = lv_obj_create(nullptr);
    lv_obj_set_size(_camera_screen, lv_display_get_horizontal_resolution(lv_display_get_default()), 
                                    lv_display_get_vertical_resolution(lv_display_get_default()));
    lv_obj_set_style_bg_color(_camera_screen, lv_color_black(), 0);
    
    // カメラプレビュー用キャンバス（シンプルに画面サイズで作成）
    _camera_preview = lv_canvas_create(_camera_screen);
    lv_obj_set_size(_camera_preview, CANVAS_WIDTH, CANVAS_HEIGHT);
    lv_obj_align(_camera_preview, LV_ALIGN_CENTER, 0, 0);
    
    // HALがバッファを設定するので、ここでは初期化のみ
    lv_canvas_fill_bg(_camera_preview, lv_color_black(), LV_OPA_COVER);
    
    // 撮影ボタン（カメラプレビューの上に重ねて表示）
    _capture_btn = lv_btn_create(_camera_screen);
    lv_obj_set_size(_capture_btn, 100, 60);  // 少し大きくして押しやすく
    lv_obj_align(_capture_btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_add_event_cb(_capture_btn, captureBtnEventHandler, LV_EVENT_CLICKED, this);
    lv_obj_move_foreground(_capture_btn);  // 前面に移動
    
    // 撮影ボタンのスタイルを目立つようにする
    lv_obj_set_style_bg_color(_capture_btn, lv_color_hex(0xFF4444), 0);
    lv_obj_set_style_radius(_capture_btn, 30, 0);
    
    lv_obj_t* capture_label = lv_label_create(_capture_btn);
    lv_label_set_text(capture_label, "CAPTURE");
    lv_obj_center(capture_label);
    lv_obj_set_style_text_color(capture_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(capture_label, &lv_font_montserrat_18, 0);
    
    // 戻るボタン（カメラプレビューの上に重ねて表示）
    _camera_back_btn = lv_btn_create(_camera_screen);
    lv_obj_set_size(_camera_back_btn, 80, 50);
    lv_obj_align(_camera_back_btn, LV_ALIGN_BOTTOM_LEFT, 20, -20);
    lv_obj_add_event_cb(_camera_back_btn, backBtnEventHandler, LV_EVENT_CLICKED, this);
    lv_obj_move_foreground(_camera_back_btn);  // 前面に移動
    
    lv_obj_t* camera_back_label = lv_label_create(_camera_back_btn);
    lv_label_set_text(camera_back_label, "Back");
    lv_obj_center(camera_back_label);
    
    // カメラ情報表示ラベル
    _camera_info_label = lv_label_create(_camera_screen);
    lv_obj_align(_camera_info_label, LV_ALIGN_TOP_RIGHT, -20, 20);
    lv_label_set_text(_camera_info_label, "CAMERA MODE\n1280x720");
    lv_obj_set_style_text_color(_camera_info_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(_camera_info_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_bg_color(_camera_info_label, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(_camera_info_label, LV_OPA_50, 0);
    lv_obj_set_style_pad_all(_camera_info_label, 8, 0);
    lv_obj_set_style_radius(_camera_info_label, 5, 0);
    lv_obj_move_foreground(_camera_info_label);  // 前面に移動
}

void AppDrawingCamera::initColorPalette()
{
    LvglLockGuard lock;
    
    // カラーパレットコンテナ
    _color_palette = lv_obj_create(_main_screen);
    lv_obj_set_size(_color_palette, 400, 60);
    lv_obj_align(_color_palette, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_bg_color(_color_palette, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_width(_color_palette, 2, 0);
    lv_obj_set_style_border_color(_color_palette, lv_color_white(), 0);
    lv_obj_set_style_radius(_color_palette, 10, 0);
    lv_obj_move_foreground(_color_palette);  // 前面に移動
    
    // カラーパレットの色設定
    lv_color_t colors[] = {
        lv_color_white(),
        lv_color_black(),
        lv_color_hex(0xFF0000), // 赤
        lv_color_hex(0x00FF00), // 緑
        lv_color_hex(0x0000FF), // 青
        lv_color_hex(0xFFFF00), // 黄
        lv_color_hex(0xFF00FF), // マゼンタ
        lv_color_hex(0x00FFFF), // シアン
        lv_color_hex(0x800080), // 紫
        lv_color_hex(0xFFA500)  // オレンジ
    };
    
    int color_count = sizeof(colors) / sizeof(colors[0]);
    int btn_size = 35;
    int spacing = (400 - (color_count * btn_size)) / (color_count + 1);
    
    for (int i = 0; i < color_count; i++) {
        lv_obj_t* color_btn = lv_btn_create(_color_palette);
        lv_obj_set_size(color_btn, btn_size, btn_size);
        lv_obj_set_pos(color_btn, spacing + i * (btn_size + spacing), 12);
        lv_obj_set_style_bg_color(color_btn, colors[i], 0);
        lv_obj_set_style_border_width(color_btn, 2, 0);
        lv_obj_set_style_border_color(color_btn, lv_color_hex(0x666666), 0);
        lv_obj_set_style_radius(color_btn, btn_size / 2, 0);
        lv_obj_add_event_cb(color_btn, colorPaletteEventHandler, LV_EVENT_CLICKED, this);
        
        // 色データを保存
        lv_obj_set_user_data(color_btn, &colors[i]);
    }
}

void AppDrawingCamera::canvasEventHandler(lv_event_t* e)
{
    AppDrawingCamera* app = static_cast<AppDrawingCamera*>(lv_event_get_user_data(e));
    lv_obj_t* canvas = static_cast<lv_obj_t*>(lv_event_get_target(e));
    
    lv_point_t point;
    lv_indev_get_point(lv_indev_get_act(), &point);
    
    // キャンバス座標に変換
    lv_coord_t canvas_x = point.x - lv_obj_get_x(canvas);
    lv_coord_t canvas_y = point.y - lv_obj_get_y(canvas);
    
    // UI要素の領域をチェック（描画を避ける）
    bool is_ui_area = false;
    
    // カラーパレット領域（上部）
    if (canvas_y >= 20 && canvas_y <= 80 && canvas_x >= 440 && canvas_x <= 840) {
        is_ui_area = true;
    }
    
    // ボタン領域（下部）
    if (canvas_y >= CANVAS_HEIGHT - 60) {
        is_ui_area = true;
    }
    
    // キャンバス範囲内かつUI領域外でのみ描画
    if (canvas_x >= 0 && canvas_x < CANVAS_WIDTH && canvas_y >= 0 && canvas_y < CANVAS_HEIGHT && !is_ui_area) {
        app->drawOnCanvas(canvas_x, canvas_y);
    }
}

void AppDrawingCamera::colorPaletteEventHandler(lv_event_t* e)
{
    AppDrawingCamera* app = static_cast<AppDrawingCamera*>(lv_event_get_user_data(e));
    lv_obj_t* btn = static_cast<lv_obj_t*>(lv_event_get_target(e));
    
    // 選択された色を取得
    lv_color_t* color = static_cast<lv_color_t*>(lv_obj_get_user_data(btn));
    if (color) {
        app->_current_color = *color;
        mclog::tagInfo("DrawingCamera", "Color changed");
    }
}

void AppDrawingCamera::cameraBtnEventHandler(lv_event_t* e)
{
    AppDrawingCamera* app = static_cast<AppDrawingCamera*>(lv_event_get_user_data(e));
    
    // カメラが既に使用中でないかチェック
    if (!GetHAL()->isCameraCapturing()) {
        app->switchToCameraMode();
    } else {
        mclog::tagWarn("DrawingCamera", "Camera is already in use");
    }
}

void AppDrawingCamera::captureBtnEventHandler(lv_event_t* e)
{
    AppDrawingCamera* app = static_cast<AppDrawingCamera*>(lv_event_get_user_data(e));
    app->capturePhoto();
}

void AppDrawingCamera::backBtnEventHandler(lv_event_t* e)
{
    AppDrawingCamera* app = static_cast<AppDrawingCamera*>(lv_event_get_user_data(e));
    
    if (app->_current_state == STATE_CAMERA_PREVIEW) {
        app->switchToDrawingMode();
    } else {
        // アプリを終了
        app->close();
    }
}

void AppDrawingCamera::clearBtnEventHandler(lv_event_t* e)
{
    AppDrawingCamera* app = static_cast<AppDrawingCamera*>(lv_event_get_user_data(e));
    app->clearCanvas();
}

void AppDrawingCamera::drawOnCanvas(lv_coord_t x, lv_coord_t y)
{
    LvglLockGuard lock;
    
    // ブラシで円を描画（LVGL v9ではより直接的な方法を使用）
    lv_area_t area;
    area.x1 = x - BRUSH_SIZE / 2;
    area.y1 = y - BRUSH_SIZE / 2;
    area.x2 = x + BRUSH_SIZE / 2;
    area.y2 = y + BRUSH_SIZE / 2;
    
    // キャンバスバッファに直接描画
    lv_draw_buf_t* buf = lv_canvas_get_draw_buf(_canvas);
    if (buf) {
        // 簡単な円形ブラシの実装
        for (int dy = -BRUSH_SIZE/2; dy <= BRUSH_SIZE/2; dy++) {
            for (int dx = -BRUSH_SIZE/2; dx <= BRUSH_SIZE/2; dx++) {
                if (dx*dx + dy*dy <= (BRUSH_SIZE/2)*(BRUSH_SIZE/2)) {
                    int px = x + dx;
                    int py = y + dy;
                    if (px >= 0 && px < CANVAS_WIDTH && py >= 0 && py < CANVAS_HEIGHT) {
                        lv_canvas_set_px(_canvas, px, py, _current_color, LV_OPA_COVER);
                    }
                }
            }
        }
    }
}

void AppDrawingCamera::clearCanvas()
{
    LvglLockGuard lock;
    
    if (_has_background_image) {
        // 背景画像がある場合は背景を再設定
        setBackgroundImage();
    } else {
        // 白で塗りつぶし
        lv_canvas_fill_bg(_canvas, lv_color_white(), LV_OPA_COVER);
    }
}

void AppDrawingCamera::setBackgroundImage()
{
    LvglLockGuard lock;
    
    if (_camera_preview && _canvas) {
        mclog::tagInfo(getAppInfo().name, "Copying camera image to canvas background");
        
        // シンプルなピクセル単位コピー（4ピクセル毎にサンプリング）
        int step = 4;
        int copied_pixels = 0;
        
        for (int y = 0; y < CANVAS_HEIGHT; y += step) {
            for (int x = 0; x < CANVAS_WIDTH; x += step) {
                // カメラプレビューから色を取得
                lv_color32_t pixel_color32 = lv_canvas_get_px(_camera_preview, x, y);
                lv_color_t pixel_color = lv_color_make(pixel_color32.red, pixel_color32.green, pixel_color32.blue);
                
                // 4x4ブロックを同じ色で塗りつぶし
                for (int dy = 0; dy < step && (y + dy) < CANVAS_HEIGHT; dy++) {
                    for (int dx = 0; dx < step && (x + dx) < CANVAS_WIDTH; dx++) {
                        lv_canvas_set_px(_canvas, x + dx, y + dy, pixel_color, LV_OPA_COVER);
                    }
                }
                copied_pixels++;
            }
            
            // 進行状況（100行毎）
            if (y % 100 == 0) {
                mclog::tagInfo(getAppInfo().name, "Copying line %d/%d", y, CANVAS_HEIGHT);
            }
        }
        
        _has_background_image = true;
        mclog::tagInfo(getAppInfo().name, "Camera image copied successfully (%d blocks)", copied_pixels);
    } else {
        mclog::tagError(getAppInfo().name, "Camera preview or canvas is null");
    }
}

void AppDrawingCamera::switchToDrawingMode()
{
    LvglLockGuard lock;
    
    _current_state = STATE_DRAWING;
    
    // カメラキャプチャを停止
    if (GetHAL()->isCameraCapturing()) {
        GetHAL()->stopCameraCapture();
    }
    
    // 描画画面を表示
    lv_screen_load_anim(_main_screen, LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, false);
    
    mclog::tagInfo(getAppInfo().name, "Switched to drawing mode");
}

void AppDrawingCamera::switchToCameraMode()
{
    LvglLockGuard lock;
    
    _current_state = STATE_CAMERA_PREVIEW;
    
    // カメラ画面を表示
    lv_screen_load_anim(_camera_screen, LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, false);
    
    // カメラキャプチャを開始
    GetHAL()->startCameraCapture(_camera_preview);
    mclog::tagInfo(getAppInfo().name, "Camera capture started");
    
    mclog::tagInfo(getAppInfo().name, "Switched to camera mode");
}

void AppDrawingCamera::capturePhoto()
{
    LvglLockGuard lock;
    
    _current_state = STATE_CAMERA_CAPTURE;
    
    mclog::tagInfo(getAppInfo().name, "Capturing photo...");
    
    // カメラが実際にキャプチャしているかチェック
    if (!GetHAL()->isCameraCapturing()) {
        mclog::tagWarn(getAppInfo().name, "Camera is not capturing, cannot take photo");
        switchToDrawingMode();
        return;
    }
    
    // 撮影した画像を背景として設定
    setBackgroundImage();
    
    // カメラキャプチャを停止
    GetHAL()->stopCameraCapture();
    
    // 描画モードに戻る
    switchToDrawingMode();
    
    mclog::tagInfo(getAppInfo().name, "Photo captured and set as background");
}

AppDrawingCamera* AppDrawingCamera::getAppInstance(lv_event_t* e)
{
    return static_cast<AppDrawingCamera*>(lv_event_get_user_data(e));
}
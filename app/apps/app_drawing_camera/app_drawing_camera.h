/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <mooncake.h>
#include <lvgl.h>
#include <memory>

/**
 * @brief Drawing Camera App - お絵描きカメラアプリ
 * 
 * Features:
 * - タッチでキャンバスに描画
 * - カラーパレットで色選択
 * - カメラで撮影して背景に設定
 */
class AppDrawingCamera : public mooncake::AppAbility {
public:
    AppDrawingCamera();

    // ライフサイクル
    void onCreate() override;
    void onOpen() override;
    void onRunning() override;
    void onClose() override;

private:
    // UI要素
    lv_obj_t* _main_screen = nullptr;
    lv_obj_t* _canvas = nullptr;
    lv_obj_t* _color_palette = nullptr;
    lv_obj_t* _camera_btn = nullptr;
    lv_obj_t* _back_btn = nullptr;
    lv_obj_t* _clear_btn = nullptr;
    
    // カメラモード用UI
    lv_obj_t* _camera_screen = nullptr;
    lv_obj_t* _camera_preview = nullptr; // キャンバスとして使用
    lv_obj_t* _capture_btn = nullptr;
    lv_obj_t* _camera_back_btn = nullptr;
    lv_obj_t* _camera_info_label = nullptr;
    
    // 描画用データ
    lv_draw_buf_t* _canvas_buffer = nullptr;
    lv_color_t _current_color = lv_color_white();
    static constexpr int CANVAS_WIDTH = 720;
    static constexpr int CANVAS_HEIGHT = 1280;
    static constexpr int BRUSH_SIZE = 20;
    
    // 状態管理
    enum AppState {
        STATE_DRAWING,
        STATE_CAMERA_PREVIEW,
        STATE_CAMERA_CAPTURE
    };
    AppState _current_state = STATE_DRAWING;
    bool _has_background_image = false;
    
    // タッチ描画の補完用
    bool _is_drawing = false;
    lv_coord_t _last_draw_x = -1;
    lv_coord_t _last_draw_y = -1;
    
    // 初期化メソッド
    void initDrawingScreen();
    void initCameraScreen();
    void initColorPalette();
    
    // イベントハンドラ
    static void canvasEventHandler(lv_event_t* e);
    static void colorPaletteEventHandler(lv_event_t* e);
    static void cameraBtnEventHandler(lv_event_t* e);
    static void captureBtnEventHandler(lv_event_t* e);
    static void backBtnEventHandler(lv_event_t* e);
    static void clearBtnEventHandler(lv_event_t* e);
    
    // 描画メソッド
    void drawOnCanvas(lv_coord_t x, lv_coord_t y);
    void drawLine(lv_coord_t x1, lv_coord_t y1, lv_coord_t x2, lv_coord_t y2);
    void clearCanvas();
    void setBackgroundImage();
    
    // 状態切り替え
    void switchToDrawingMode();
    void switchToCameraMode();
    void capturePhoto();
    
    // ユーティリティ
    AppDrawingCamera* getAppInstance(lv_event_t* e);
};
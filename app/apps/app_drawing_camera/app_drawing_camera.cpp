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
#include <cmath>    // sqrt用

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
        uint32_t now                = GetHAL()->millis();

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
    lv_obj_add_event_cb(_canvas, canvasEventHandler, LV_EVENT_RELEASED, this);
    lv_obj_add_flag(_canvas, LV_OBJ_FLAG_CLICKABLE);

    // カラーパレットコンテナ（下部に配置、縦向き最適化）
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
        lv_color_white(),       lv_color_black(),
        lv_color_hex(0xFF0000),  // 赤
        lv_color_hex(0x00FF00),  // 緑
        lv_color_hex(0x0000FF),  // 青
        lv_color_hex(0xFFFF00),  // 黄
        lv_color_hex(0xFF00FF),  // マゼンタ
        lv_color_hex(0x00FFFF),  // シアン
        lv_color_hex(0x800080),  // 紫
        lv_color_hex(0xFFA500)   // オレンジ
    };

    int color_count = sizeof(colors) / sizeof(colors[0]);
    int btn_size    = 35;
    int spacing     = (400 - (color_count * btn_size)) / (color_count + 1);

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

    // カメラボタン（上部右に配置）
    _camera_btn = lv_btn_create(_main_screen);
    lv_obj_set_size(_camera_btn, 80, 40);
    lv_obj_align(_camera_btn, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
    lv_obj_add_event_cb(_camera_btn, cameraBtnEventHandler, LV_EVENT_CLICKED, this);
    lv_obj_move_foreground(_camera_btn);  // 前面に移動

    lv_obj_t* camera_label = lv_label_create(_camera_btn);
    lv_label_set_text(camera_label, "Camera");
    lv_obj_center(camera_label);

    // クリアボタン（上部左に配置）
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

    // 撮影ボタン（上部中央に配置）
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

    // 戻るボタン（上部左に配置）
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

void AppDrawingCamera::canvasEventHandler(lv_event_t* e)
{
    AppDrawingCamera* app      = static_cast<AppDrawingCamera*>(lv_event_get_user_data(e));
    lv_obj_t* canvas           = static_cast<lv_obj_t*>(lv_event_get_target(e));
    lv_event_code_t event_code = lv_event_get_code(e);

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

    // キャンバス範囲内かつUI領域外でのみ処理
    if (canvas_x >= 0 && canvas_x < CANVAS_WIDTH && canvas_y >= 0 && canvas_y < CANVAS_HEIGHT && !is_ui_area) {
        if (event_code == LV_EVENT_PRESSED) {
            // タッチ開始 - 最初の点を描画
            app->_is_drawing  = true;
            app->_last_draw_x = canvas_x;
            app->_last_draw_y = canvas_y;
            app->drawOnCanvas(canvas_x, canvas_y);
        } else if (event_code == LV_EVENT_PRESSING) {
            // タッチ中 - 前回の点から現在の点まで線を描画
            if (app->_is_drawing && app->_last_draw_x >= 0 && app->_last_draw_y >= 0) {
                app->drawLine(app->_last_draw_x, app->_last_draw_y, canvas_x, canvas_y);
            }
            app->_last_draw_x = canvas_x;
            app->_last_draw_y = canvas_y;
        }
    }

    if (event_code == LV_EVENT_RELEASED) {
        // タッチ終了
        app->_is_drawing  = false;
        app->_last_draw_x = -1;
        app->_last_draw_y = -1;
    }
}

void AppDrawingCamera::colorPaletteEventHandler(lv_event_t* e)
{
    AppDrawingCamera* app = static_cast<AppDrawingCamera*>(lv_event_get_user_data(e));
    lv_obj_t* btn         = static_cast<lv_obj_t*>(lv_event_get_target(e));

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
    // 究極高速化：事前計算 + DMA風メモリ操作
    lv_draw_buf_t* buf = lv_canvas_get_draw_buf(_canvas);
    if (!buf || !buf->data) return;

    // 高速定数
    static const int radius      = BRUSH_SIZE / 2;
    static const int diameter    = BRUSH_SIZE;
    static uint16_t cached_color = 0;
    static uint16_t color16      = 0;

    // 色変換キャッシュ（RGB565変換は重い）
    uint16_t current_color16 = lv_color_to_u16(_current_color);
    if (cached_color != current_color16) {
        cached_color = current_color16;
        color16      = current_color16;
    }

    // 高速バッファアクセス
    uint32_t buf_width   = buf->header.w;
    uint16_t* pixel_data = (uint16_t*)buf->data;

    // 超高速範囲チェック（分岐予測最適化）
    if (__builtin_expect(
            x < radius || y < radius || x >= (int)(buf_width - radius) || y >= (int)(buf->header.h - radius), 0)) {
        return;
    }

    // 究極最適化：正方形ブラシ（円計算を省略）
    int start_y = y - radius;
    int end_y   = y + radius;
    int start_x = x - radius;

    // 行ごとに超高速メモリ書き込み
    for (int py = start_y; py <= end_y; py++) {
        uint16_t* row_start = pixel_data + py * buf_width + start_x;

        // memset風の超高速一括書き込み（アンロールループ）
        uint16_t* ptr = row_start;
        int remaining = diameter;

        // 8ピクセルずつ展開（CPU パイプライン最適化）
        while (remaining >= 8) {
            ptr[0] = color16;
            ptr[1] = color16;
            ptr[2] = color16;
            ptr[3] = color16;
            ptr[4] = color16;
            ptr[5] = color16;
            ptr[6] = color16;
            ptr[7] = color16;
            ptr += 8;
            remaining -= 8;
        }

        // 残りのピクセル
        while (remaining > 0) {
            *ptr++ = color16;
            remaining--;
        }
    }

    // 画面更新最適化（部分更新のみ）
    lv_area_t update_area;
    update_area.x1 = x - radius;
    update_area.y1 = y - radius;
    update_area.x2 = x + radius;
    update_area.y2 = y + radius;
    lv_obj_invalidate_area(_canvas, &update_area);
}

void AppDrawingCamera::drawLine(lv_coord_t x1, lv_coord_t y1, lv_coord_t x2, lv_coord_t y2)
{
    // 点の間隔を計算
    int distance = abs(x2 - x1) + abs(y2 - y1);

    // 距離が短い場合は直接円を描画
    if (distance <= BRUSH_SIZE / 2) {
        drawOnCanvas(x2, y2);
        return;
    }

    // ブレゼンハムアルゴリズムで等間隔に円を描画
    int dx  = abs(x2 - x1);
    int dy  = abs(y2 - y1);
    int sx  = x1 < x2 ? 1 : -1;
    int sy  = y1 < y2 ? 1 : -1;
    int err = dx - dy;

    int x          = x1;
    int y          = y1;
    int step_count = 0;

    while (true) {
        // 一定間隔で円を描画（重複を防ぐ）
        if (step_count % (BRUSH_SIZE / 4) == 0) {
            drawOnCanvas(x, y);
        }

        if (x == x2 && y == y2) break;

        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
        step_count++;
    }

    // 最終点も描画
    drawOnCanvas(x2, y2);
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

        // カメラプレビューとキャンバスのドローバッファを取得
        lv_draw_buf_t* camera_buf = lv_canvas_get_draw_buf(_camera_preview);
        lv_draw_buf_t* canvas_buf = lv_canvas_get_draw_buf(_canvas);

        if (camera_buf && canvas_buf) {
            // バッファサイズを確認
            uint32_t camera_width  = camera_buf->header.w;
            uint32_t camera_height = camera_buf->header.h;
            uint32_t canvas_width  = canvas_buf->header.w;
            uint32_t canvas_height = canvas_buf->header.h;

            mclog::tagInfo(getAppInfo().name, "Camera buffer: %dx%d, Canvas buffer: %dx%d", camera_width, camera_height,
                           canvas_width, canvas_height);

            // 両方のバッファが同じサイズの場合は直接コピー
            if (camera_width == canvas_width && camera_height == canvas_height) {
                uint8_t* camera_data = camera_buf->data;
                uint8_t* canvas_data = canvas_buf->data;

                if (camera_data && canvas_data) {
                    // RGB565フォーマットを前提として、1ピクセル = 2バイト
                    uint32_t buffer_size = camera_width * camera_height * 2;
                    memcpy(canvas_data, camera_data, buffer_size);
                    mclog::tagInfo(getAppInfo().name, "Camera image copied directly (%d bytes)", buffer_size);
                } else {
                    mclog::tagError(getAppInfo().name, "Failed to get buffer data pointers");
                    return;
                }
            } else {
                // サイズが異なる場合はピクセル単位でコピー（1:1スケール）
                uint32_t copy_width  = (camera_width < canvas_width) ? camera_width : canvas_width;
                uint32_t copy_height = (camera_height < canvas_height) ? camera_height : canvas_height;

                mclog::tagInfo(getAppInfo().name, "Copying %dx%d pixels", copy_width, copy_height);

                for (uint32_t y = 0; y < copy_height; y++) {
                    for (uint32_t x = 0; x < copy_width; x++) {
                        lv_color32_t pixel_color32 = lv_canvas_get_px(_camera_preview, x, y);
                        lv_color_t pixel_color =
                            lv_color_make(pixel_color32.red, pixel_color32.green, pixel_color32.blue);
                        lv_canvas_set_px(_canvas, x, y, pixel_color, LV_OPA_COVER);
                    }

                    // 進行状況（100行毎）
                    if (y % 100 == 0) {
                        mclog::tagInfo(getAppInfo().name, "Copying line %d/%d", y, copy_height);
                    }
                }
                mclog::tagInfo(getAppInfo().name, "Camera image copied pixel by pixel");
            }

            // キャンバスを無効化して再描画を促す
            lv_obj_invalidate(_canvas);

        } else {
            mclog::tagError(getAppInfo().name, "Failed to get draw buffers");
            return;
        }

        _has_background_image = true;
        mclog::tagInfo(getAppInfo().name, "Camera image set as background successfully");
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
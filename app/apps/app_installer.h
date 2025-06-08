/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <mooncake.h>
#include <memory>
#include <hal/hal.h>
#include "app_drawing_camera/app_drawing_camera.h"
/* Header files locator (Don't remove) */

// Simplified startup - no animation for drawing app
inline void on_startup_anim()
{
    // Skip startup animation for drawing app
}

/**
 * @brief App 安装回调
 *
 * @param mooncake
 */
inline void on_install_apps()
{
    // Install drawing camera app only
    mooncake::GetMooncake().installApp(std::make_unique<AppDrawingCamera>());
    /* Install app locator (Don't remove) */
}

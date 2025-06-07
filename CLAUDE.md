# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is the M5Tab5 user demo application - a dual-platform project that can run on both desktop (Linux/SDL2) and embedded ESP32-P4 hardware. The project uses LVGL for GUI, Mooncake for application framework, and implements a Hardware Abstraction Layer (HAL) pattern for cross-platform compatibility.

## Build System and Commands

### Initial Setup
```bash
# Fetch all dependencies first (required before any build)
python ./fetch_repos.py
```

### Desktop Build (Development/Testing)
```bash
# Install dependencies (Ubuntu/Debian)
sudo apt install build-essential cmake libsdl2-dev

# Build
mkdir build && cd build
cmake .. && make -j8

# Run
./desktop/app_desktop_build
```

### ESP32-P4 Build (Target Hardware)
```bash
# Prerequisites: ESP-IDF v5.4.1 installed
cd platforms/tab5

# Build
idf.py build

# Flash to device
idf.py flash

# Monitor serial output
idf.py monitor
```

## Development Tools

### App Generation
```bash
# Create a new app from template
cd app/apps
python app_generator.py
```

## Architecture

### Cross-Platform Design
- **HAL (Hardware Abstraction Layer)**: `app/hal/hal.h` defines the interface with implementations in `platforms/desktop/hal/` and `platforms/tab5/main/hal/`
- **App Layer**: `app/` contains platform-agnostic application code
- **Platform Specific**: `platforms/desktop/` and `platforms/tab5/` contain platform-specific implementations

### Key Components
- **Mooncake Framework**: Application lifecycle management and UI framework
- **LVGL v9.2.0**: Graphics library for UI rendering  
- **HAL Interface**: Provides unified API for display, camera, IMU, power, audio, WiFi, and other hardware features
- **Dependency Management**: `repos.json` and `fetch_repos.py` handle external dependencies

### Application Structure
- Apps are located in `app/apps/` with each app having its own directory
- `app_launcher` provides the main UI for hardware testing and feature demonstration
- Shared utilities in `app/apps/utils/` for audio, math, and UI components
- Assets (images, fonts) stored in `app/assets/`
- App registration via `app/apps/app_installer.h`

### LVGL v9 API Notes
When working with canvas/draw buffers, use the correct v9 API:
```cpp
// Correct v9 API for buffer access
lv_draw_buf_t* buf = lv_canvas_get_draw_buf(canvas);
uint32_t width = buf->header.w;   // NOT lv_draw_buf_get_width()
uint32_t height = buf->header.h;  // NOT lv_draw_buf_get_height()
uint8_t* data = buf->data;        // NOT lv_draw_buf_get_buf()
```

### Hardware Features (ESP32-P4)
- 1280x720 display with touch input
- Camera sensor support
- IMU (BMI270), power monitoring (INA226) 
- Audio recording/playback with multiple microphones
- WiFi, USB-A/C, SD card, RTC
- GPIO and I2C interfaces

The HAL pattern allows the same application code to run on desktop for development and on hardware for production deployment.
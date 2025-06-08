# M5Tab5 Drawing Camera App

Interactive drawing and camera application for [M5Tab5](https://docs.m5stack.com/en/products/sku/k145) featuring real-time drawing with camera integration.

## Features

- **Drawing Canvas**: Touch-based drawing with customizable brush colors and sizes
- **Camera Integration**: Live camera preview and photo capture functionality  
- **Dual-Platform**: Runs on both desktop (SDL2) and ESP32-P4 hardware
- **Hardware Testing**: Comprehensive hardware validation through app launcher

## Build

### Fetch Dependencies

```bash
python ./fetch_repos.py
```

## Desktop Build

#### Tool Chains

```bash
sudo apt install build-essential cmake libsdl2-dev
```

#### Build

```bash
mkdir build && cd build
```

```bash
cmake .. && make -j8
```

#### Run

```bash
./desktop/app_desktop_build
```

## IDF Build

#### Tool Chains

[ESP-IDF v5.4.1](https://docs.espressif.com/projects/esp-idf/en/v5.4.1/esp32s3/index.html)

#### Build

```bash
cd platforms/tab5
```

```bash
idf.py build
```

#### Flash

```bash
idf.py flash
```

## Acknowledgments

This project is based on the official [M5Stack M5Tab5-UserDemo](https://github.com/m5stack/M5Tab5-UserDemo) repository and has been extended with drawing and camera integration features.

### Original Source
- **M5Stack M5Tab5-UserDemo**: https://github.com/m5stack/M5Tab5-UserDemo - Official user demo for M5Tab5 hardware evaluation

### Libraries and Dependencies
This project references the following open-source libraries and resources:

- https://github.com/lvgl/lvgl
- https://www.heroui.com
- https://github.com/Forairaaaaa/smooth_ui_toolkit
- https://github.com/Forairaaaaa/mooncake
- https://github.com/Forairaaaaa/mooncake_log
- https://github.com/alexreinert/piVCCU/blob/master/kernel/rtc-rx8130.c
- https://components.espressif.com/components/espressif/esp_cam_sensor
- https://components.espressif.com/components/espressif/esp_ipa
- https://components.espressif.com/components/espressif/esp_sccb_intf
- https://components.espressif.com/components/espressif/esp_video
- https://components.espressif.com/components/espressif/esp_lvgl_port
- https://github.com/jarzebski/Arduino-INA226
- https://github.com/boschsensortec/BMI270_SensorAPI

# Phase 1: 基础重构 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 移除 3D 立方体，简化屏幕显示，保留 WiFi + NTP + 天气功能，为后续功能扩展做准备。

**Architecture:** 清理现有代码，移除 cube.c/h 模块，简化 display.c 只显示角度和时间，保留 web_server 和 ntp_time 功能。

**Tech Stack:** ESP-IDF v5.4, LVGL 8.4, ILI9341, MPU6050, WiFi

---

## 文件变更清单

| 文件 | 操作 | 说明 |
|------|------|------|
| main/cube.c | 删除 | 3D 立方体代码 |
| main/cube.h | 删除 | 3D 立方体头文件 |
| main/display.c | 修改 | 移除立方体，简化 UI |
| main/display.h | 保持 | 接口不变 |
| main/main.c | 修改 | 移除 cube 引用 |
| main/CMakeLists.txt | 修改 | 移除 cube.c |
| main/lcd_test.c | 删除 | 测试代码 |

---

### Task 1: 清理旧代码

**Files:**
- Delete: `main/cube.c`
- Delete: `main/cube.h`
- Delete: `main/lcd_test.c`

- [ ] **Step 1: 删除 3D 立方体文件**

```bash
rm main/cube.c main/cube.h main/lcd_test.c
```

- [ ] **Step 2: 更新 CMakeLists.txt**

修改 `main/CMakeLists.txt`，移除 cube.c：

```cmake
idf_component_register(SRCS "main.c" "mpu6050.c" "display.c" "web_server.c"
                       INCLUDE_DIRS ".")
```

- [ ] **Step 3: 更新 main.c，移除 cube 引用**

修改 `main/main.c`，移除 `#include "cube.h"` 相关代码（如果有的话）。

- [ ] **Step 4: 验证编译通过**

```powershell
cd D:\STM32CubeFile\IMU-Gesture-Visualizer
Remove-Item -Recurse -Force build
$env:PATH = "C:\Users\o\.espressif\tools\xtensa-esp-elf\esp-14.2.0_20260121\xtensa-esp-elf\bin;C:\Users\o\.espressif\tools\ninja\1.12.1;C:\Users\o\.espressif\tools\cmake\3.30.5\bin;" + $env:PATH
& "C:\Users\o\.espressif\python_env\idf5.4_py3.13_env\Scripts\python.exe" "D:\ESP-IDF\.espressif\v5.4.4\esp-idf\tools\idf.py" build
```

Expected: BUILD SUCCESSFUL

- [ ] **Step 5: 提交**

```bash
git add -A
git commit -m "refactor: remove 3D cube and lcd_test code"
```

---

### Task 2: 简化显示模块

**Files:**
- Modify: `main/display.c`

- [ ] **Step 1: 移除 cube.h 引用**

在 `main/display.c` 中移除 `#include "cube.h"`。

- [ ] **Step 2: 修改 create_ui 函数**

简化界面，只保留核心元素：

```c
static void create_ui(void) {
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);

    // 标题
    lv_obj_t *label_title = lv_label_create(scr);
    lv_label_set_text(label_title, "Smart Watch");
    lv_obj_set_style_text_color(label_title, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_title, &lv_font_montserrat_14, 0);
    lv_obj_align(label_title, LV_ALIGN_TOP_MID, 0, 5);

    // 温度
    label_temp = lv_label_create(scr);
    lv_label_set_text(label_temp, "Temp: --°C");
    lv_obj_set_style_text_color(label_temp, lv_color_make(255, 100, 0), 0);
    lv_obj_set_style_text_font(label_temp, &lv_font_montserrat_14, 0);
    lv_obj_align(label_temp, LV_ALIGN_LEFT_MID, 20, -60);

    // 湿度
    label_humi = lv_label_create(scr);
    lv_label_set_text(label_humi, "Humi: --%");
    lv_obj_set_style_text_color(label_humi, lv_color_make(0, 128, 255), 0);
    lv_obj_set_style_text_font(label_humi, &lv_font_montserrat_14, 0);
    lv_obj_align(label_humi, LV_ALIGN_LEFT_MID, 20, -40);

    // Pitch
    label_pitch = lv_label_create(scr);
    lv_label_set_text(label_pitch, "Pitch: 0.0");
    lv_obj_set_style_text_color(label_pitch, lv_color_make(0, 255, 0), 0);
    lv_obj_set_style_text_font(label_pitch, &lv_font_montserrat_14, 0);
    lv_obj_align(label_pitch, LV_ALIGN_LEFT_MID, 20, 0);

    // Pitch 进度条
    bar_pitch = lv_bar_create(scr);
    lv_obj_set_size(bar_pitch, 200, 10);
    lv_bar_set_range(bar_pitch, 0, 100);
    lv_bar_set_value(bar_pitch, 50, LV_ANIM_OFF);
    lv_obj_align(bar_pitch, LV_ALIGN_LEFT_MID, 20, 20);
    lv_obj_set_style_bg_color(bar_pitch, lv_color_make(50, 50, 50), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar_pitch, lv_color_make(0, 255, 0), LV_PART_INDICATOR);

    // Roll
    label_roll = lv_label_create(scr);
    lv_label_set_text(label_roll, "Roll: 0.0");
    lv_obj_set_style_text_color(label_roll, lv_color_make(0, 128, 255), 0);
    lv_obj_set_style_text_font(label_roll, &lv_font_montserrat_14, 0);
    lv_obj_align(label_roll, LV_ALIGN_LEFT_MID, 20, 40);

    // Roll 进度条
    bar_roll = lv_bar_create(scr);
    lv_obj_set_size(bar_roll, 200, 10);
    lv_bar_set_range(bar_roll, 0, 100);
    lv_bar_set_value(bar_roll, 50, LV_ANIM_OFF);
    lv_obj_align(bar_roll, LV_ALIGN_LEFT_MID, 20, 60);
    lv_obj_set_style_bg_color(bar_roll, lv_color_make(50, 50, 50), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar_roll, lv_color_make(0, 128, 255), LV_PART_INDICATOR);

    // 天气
    label_weather = lv_label_create(scr);
    lv_label_set_text(label_weather, "Weather: --");
    lv_obj_set_style_text_color(label_weather, lv_color_make(255, 255, 0), 0);
    lv_obj_set_style_text_font(label_weather, &lv_font_montserrat_14, 0);
    lv_obj_align(label_weather, LV_ALIGN_LEFT_MID, 20, 90);

    // 时间
    label_time = lv_label_create(scr);
    lv_label_set_text(label_time, "--:--");
    lv_obj_set_style_text_color(label_time, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_time, &lv_font_montserrat_14, 0);
    lv_obj_align(label_time, LV_ALIGN_BOTTOM_MID, 0, -10);
}
```

- [ ] **Step 3: 更新 display_update 函数**

简化更新逻辑：

```c
void display_update(const mpu6050_data_t *data, float temp, float humi, const char *time_str, const char *weather_str) {
    if (!label_pitch) {
        create_ui();
    }

    // 更新温湿度
    char buf[32];
    snprintf(buf, sizeof(buf), "Temp: %.1f°C", temp);
    lv_label_set_text(label_temp, buf);

    snprintf(buf, sizeof(buf), "Humi: %.0f%%", humi);
    lv_label_set_text(label_humi, buf);

    // 更新角度
    snprintf(buf, sizeof(buf), "Pitch: %+.1f", data->pitch);
    lv_label_set_text(label_pitch, buf);
    lv_bar_set_value(bar_pitch, (int)((data->pitch + 90.0f) * 100.0f / 180.0f), LV_ANIM_OFF);

    snprintf(buf, sizeof(buf), "Roll: %+.1f", data->roll);
    lv_label_set_text(label_roll, buf);
    lv_bar_set_value(bar_roll, (int)((data->roll + 90.0f) * 100.0f / 180.0f), LV_ANIM_OFF);

    // 更新天气
    lv_label_set_text(label_weather, weather_str);

    // 更新时间
    lv_label_set_text(label_time, time_str);
}
```

- [ ] **Step 4: 更新 display.h 接口**

```c
// main/display.h
#ifndef DISPLAY_H
#define DISPLAY_H

#include "esp_err.h"
#include "mpu6050.h"

esp_err_t display_init(void);
void display_update(const mpu6050_data_t *data, float temp, float humi, const char *time_str, const char *weather_str);

#endif
```

- [ ] **Step 5: 验证编译通过**

```powershell
$env:PATH = "C:\Users\o\.espressif\tools\xtensa-esp-elf\esp-14.2.0_20260121\xtensa-esp-elf\bin;C:\Users\o\.espressif\tools\ninja\1.12.1;C:\Users\o\.espressif\tools\cmake\3.30.5\bin;" + $env:PATH
& "C:\Users\o\.espressif\python_env\idf5.4_py3.13_env\Scripts\python.exe" "D:\ESP-IDF\.espressif\v5.4.4\esp-idf\tools\idf.py" build
```

Expected: BUILD SUCCESSFUL

- [ ] **Step 6: 提交**

```bash
git add main/display.c main/display.h
git commit -m "refactor: simplify display UI, remove cube dependency"
```

---

### Task 3: 更新主程序

**Files:**
- Modify: `main/main.c`

- [ ] **Step 1: 更新 main.c**

简化主循环，准备集成新模块：

```c
// main/main.c — Smart Watch Phase 1
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "driver/i2c_master.h"
#include "mpu6050.h"
#include "display.h"
#include "web_server.h"

static const char *TAG = "MAIN";

#define I2C_SDA_PIN  32
#define I2C_SCL_PIN  33

// 全局数据
static mpu6050_data_t g_mpu_data;
static float g_temp = 0, g_humi = 0;
static char g_time_str[16] = "--:--";
static char g_weather_str[32] = "Weather: --";
static SemaphoreHandle_t g_data_mutex;

// MPU6050 任务
static void mpu_task(void *pvParameters) {
    esp_task_wdt_delete(NULL);
    i2c_master_bus_handle_t bus_handle = (i2c_master_bus_handle_t)pvParameters;
    vTaskDelay(pdMS_TO_TICKS(1000));

    mpu6050_data_t local_data;
    if (mpu6050_init(bus_handle, &local_data) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init MPU6050");
        vTaskDelete(NULL);
        return;
    }

    while (1) {
        if (mpu6050_read(&local_data) == ESP_OK) {
            if (xSemaphoreTake(g_data_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                g_mpu_data = local_data;
                xSemaphoreGive(g_data_mutex);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// WiFi 任务
static void wifi_task(void *pvParameters) {
    ESP_LOGI(TAG, "WiFi task started");
    web_server_init();
    vTaskDelete(NULL);
}

void app_main(void) {
    ESP_LOGI(TAG, "=== Smart Watch - Phase 1 ===");

    g_data_mutex = xSemaphoreCreateMutex();

    // 初始化显示
    esp_err_t ret = display_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize display");
        while (1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
    }

    // 初始化 I2C
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_1,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus_handle;
    ret = i2c_new_master_bus(&bus_config, &bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2C");
        while (1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
    }

    // MPU6050 固定到 Core 1
    xTaskCreatePinnedToCore(mpu_task, "mpu_task", 4096, bus_handle, 5, NULL, 1);

    // WiFi
    xTaskCreate(wifi_task, "wifi_task", 8192, NULL, 2, NULL);

    ESP_LOGI(TAG, "Starting main loop...");

    while (1) {
        if (xSemaphoreTake(g_data_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            display_update(&g_mpu_data, g_temp, g_humi, g_time_str, g_weather_str);
            xSemaphoreGive(g_data_mutex);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
```

- [ ] **Step 2: 验证编译通过**

```powershell
$env:PATH = "C:\Users\o\.espressif\tools\xtensa-esp-elf\esp-14.2.0_20260121\xtensa-esp-elf\bin;C:\Users\o\.espressif\tools\ninja\1.12.1;C:\Users\o\.espressif\tools\cmake\3.30.5\bin;" + $env:PATH
& "C:\Users\o\.espressif\python_env\idf5.4_py3.13_env\Scripts\python.exe" "D:\ESP-IDF\.espressif\v5.4.4\esp-idf\tools\idf.py" build
```

Expected: BUILD SUCCESSFUL

- [ ] **Step 3: 烧录测试**

```powershell
& "C:\Users\o\.espressif\python_env\idf5.4_py3.13_env\Scripts\python.exe" "D:\ESP-IDF\.espressif\v5.4.4\esp-idf\tools\idf.py" -p COM16 flash monitor
```

Expected: 屏幕显示简化界面，角度数据正常

- [ ] **Step 4: 提交**

```bash
git add main/main.c
git commit -m "refactor: update main loop for new display interface"
```

---

### Task 4: 验证整体功能

- [ ] **Step 1: 烧录并测试所有功能**

```powershell
& "C:\Users\o\.espressif\python_env\idf5.4_py3.13_env\Scripts\python.exe" "D:\ESP-IDF\.espressif\v5.4.4\esp-idf\tools\idf.py" -p COM16 flash monitor
```

验证项目：
1. 屏幕显示简化界面（温湿度、角度、时间、天气）
2. 角度数据随板子倾斜变化
3. WiFi 热点可连接
4. 网页可访问 192.168.4.1

- [ ] **Step 2: 最终提交**

```bash
git add -A
git commit -m "feat: Phase 1 complete - basic refactor ready for new features"
```

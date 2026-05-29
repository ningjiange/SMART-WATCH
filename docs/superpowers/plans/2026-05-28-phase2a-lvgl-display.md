# Phase 2a: LVGL 屏幕显示角度 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 在 ILI9341 屏幕上显示 MPU6050 的 Pitch/Roll 角度数值和进度条。

**Architecture:** 使用 ESP-IDF 组件管理器集成 LVGL + ILI9341 驱动，新建 display 模块封装屏幕初始化和 UI 更新逻辑，主循环调用 display_update() 刷新界面。

**Tech Stack:** ESP-IDF v5.4, LVGL 8.4, esp_lvgl_port, esp_lcd_ili9341, C

---

## 文件结构

```
main/
├── main.c              — 修改：添加 LVGL 初始化 + 主循环调用 display_update
├── mpu6050.c           — 不变
├── mpu6050.h           — 不变
├── display.h           — 新建：显示模块头文件
├── display.c           — 新建：LVGL 初始化 + UI 创建 + 数据更新
├── CMakeLists.txt      — 修改：添加 display.c
└── idf_component.yml   — 新建：LVGL 依赖声明
```

---

### Task 1: 添加 LVGL 组件依赖

**Files:**
- Create: `main/idf_component.yml`
- Modify: `sdkconfig.defaults`

- [ ] **Step 1: 创建 idf_component.yml**

```yaml
# main/idf_component.yml
dependencies:
  lvgl/lvgl: "~8.4"
  esp_lvgl_port: "~2.4"
  esp_lcd/esp_lcd_ili9341: "~1.1"
```

- [ ] **Step 2: 更新 sdkconfig.defaults**

在文件末尾追加 LVGL 和 SPI 配置：

```
# sdkconfig.defaults（追加内容）
# LVGL 配置
CONFIG_LV_COLOR_DEPTH_16=y
CONFIG_LV_USE_PERF_MONITOR=n
CONFIG_LV_USE_LOG=n

# SPI 配置（ILI9341 使用 SPI2）
CONFIG_SPI2=y
CONFIG_SPI_MASTER_IN_IRAM=y
```

- [ ] **Step 3: 验证编译能通过（会报 display.c 缺失，正常）**

在 PowerShell 中运行：

```powershell
$env:PATH = "C:\Users\o\.espressif\tools\xtensa-esp-elf\esp-14.2.0_20260121\xtensa-esp-elf\bin;C:\Users\o\.espressif\tools\ninja\1.12.1;C:\Users\o\.espressif\tools\cmake\3.30.5\bin;" + $env:PATH
& "C:\Users\o\.espressif\python_env\idf5.4_py3.13_env\Scripts\python.exe" "D:\ESP-IDF\.espressif\v5.4.4\esp-idf\tools\idf.py" build
```

Expected: 组件下载成功，报 `display.c` 找不到的链接错误（因为还没创建）

- [ ] **Step 4: 提交**

```bash
git add main/idf_component.yml sdkconfig.defaults
git commit -m "feat: add LVGL and ILI9341 component dependencies"
```

---

### Task 2: 创建显示模块头文件

**Files:**
- Create: `main/display.h`

- [ ] **Step 1: 创建 display.h**

```c
// main/display.h
#ifndef DISPLAY_H
#define DISPLAY_H

#include "esp_err.h"
#include "mpu6050.h"

/**
 * @brief 初始化 LVGL 和 ILI9341 屏幕
 * @return ESP_OK 成功
 */
esp_err_t display_init(void);

/**
 * @brief 更新屏幕上的角度数据
 * @param data MPU6050 数据结构体指针
 */
void display_update(const mpu6050_data_t *data);

#endif
```

- [ ] **Step 2: 提交**

```bash
git add main/display.h
git commit -m "feat: add display module header"
```

---

### Task 3: 实现显示模块

**Files:**
- Create: `main/display.c`

- [ ] **Step 1: 创建 display.c — LVGL 初始化部分**

```c
// main/display.c
#include "display.h"
#include "esp_log.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_io.h"
#include "driver/spi_master.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"

static const char *TAG = "DISPLAY";

// SPI 引脚定义
#define LCD_HOST        SPI2_HOST
#define LCD_SCLK_PIN    18
#define LCD_MOSI_PIN    23
#define LCD_MISO_PIN    19
#define LCD_DC_PIN      2
#define LCD_CS_PIN      15
#define LCD_RST_PIN     4
#define LCD_BL_PIN      -1  // 背光直接接 3.3V，不用 GPIO 控制

// 屏幕参数
#define LCD_H_RES       240
#define LCD_V_RES       320

// LVGL UI 组件（全局，方便 update 函数访问）
static lv_obj_t *label_title = NULL;
static lv_obj_t *label_pitch = NULL;
static lv_obj_t *bar_pitch = NULL;
static lv_obj_t *label_roll = NULL;
static lv_obj_t *bar_roll = NULL;
static lv_obj_t *label_status = NULL;

esp_err_t display_init(void) {
    ESP_LOGI(TAG, "Initializing display...");

    // 1. 初始化 SPI 总线
    spi_bus_config_t bus_cfg = {
        .sclk_io_num = LCD_SCLK_PIN,
        .mosi_io_num = LCD_MOSI_PIN,
        .miso_io_num = LCD_MISO_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_H_RES * 80 * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &bus_cfg, SPI_DMA_CH_AUTO));

    // 2. 初始化 LCD 面板 IO
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_cfg = {
        .dc_gpio_num = LCD_DC_PIN,
        .cs_gpio_num = LCD_CS_PIN,
        .pclk_hz = 40 * 1000 * 1000,  // 40MHz
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(LCD_HOST, &io_cfg, &io_handle));

    // 3. 初始化 ILI9341 面板
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = LCD_RST_PIN,
        .rgb_endian = LCD_RGB_ENDIAN_BGR,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(io_handle, &panel_cfg, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    // 4. 初始化 LVGL
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

    // 5. 添加 LVGL 显示
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = LCD_H_RES * 40,
        .flags = {
            .buff_dma = true,
        }
    };
    lvgl_port_add_disp(&disp_cfg);

    ESP_LOGI(TAG, "Display initialized successfully");
    return ESP_OK;
}
```

- [ ] **Step 2: 创建 display.c — UI 创建和更新函数**

在上一步的文件末尾追加：

```c
// ===== UI 创建 =====

static void create_ui(void) {
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);

    // 标题
    label_title = lv_label_create(scr);
    lv_label_set_text(label_title, "IMU Gesture Visualizer");
    lv_obj_set_style_text_color(label_title, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_title, &lv_font_montserrat_16, 0);
    lv_obj_align(label_title, LV_ALIGN_TOP_MID, 0, 10);

    // Pitch 标签
    label_pitch = lv_label_create(scr);
    lv_label_set_text(label_pitch, "Pitch: 0.0");
    lv_obj_set_style_text_color(label_pitch, lv_color_make(0, 255, 0), 0);
    lv_obj_set_style_text_font(label_pitch, &lv_font_montserrat_14, 0);
    lv_obj_align(label_pitch, LV_ALIGN_LEFT_MID, 20, -30);

    // Pitch 进度条
    bar_pitch = lv_bar_create(scr);
    lv_obj_set_size(bar_pitch, 200, 15);
    lv_bar_set_range(bar_pitch, 0, 100);
    lv_bar_set_value(bar_pitch, 0, LV_ANIM_OFF);
    lv_obj_align(bar_pitch, LV_ALIGN_LEFT_MID, 20, -10);
    lv_obj_set_style_bg_color(bar_pitch, lv_color_make(50, 50, 50), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar_pitch, lv_color_make(0, 255, 0), LV_PART_INDICATOR);

    // Roll 标签
    label_roll = lv_label_create(scr);
    lv_label_set_text(label_roll, "Roll: 0.0");
    lv_obj_set_style_text_color(label_roll, lv_color_make(0, 128, 255), 0);
    lv_obj_set_style_text_font(label_roll, &lv_font_montserrat_14, 0);
    lv_obj_align(label_roll, LV_ALIGN_LEFT_MID, 20, 20);

    // Roll 进度条
    bar_roll = lv_bar_create(scr);
    lv_obj_set_size(bar_roll, 200, 15);
    lv_bar_set_range(bar_roll, 0, 100);
    lv_bar_set_value(bar_roll, 0, LV_ANIM_OFF);
    lv_obj_align(bar_roll, LV_ALIGN_LEFT_MID, 20, 40);
    lv_obj_set_style_bg_color(bar_roll, lv_color_make(50, 50, 50), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar_roll, lv_color_make(0, 128, 255), LV_PART_INDICATOR);

    // 状态栏
    label_status = lv_label_create(scr);
    lv_label_set_text(label_status, "Mode: FREE | WiFi: --");
    lv_obj_set_style_text_color(label_status, lv_color_make(128, 128, 128), 0);
    lv_obj_set_style_text_font(label_status, &lv_font_montserrat_12, 0);
    lv_obj_align(label_status, LV_ALIGN_BOTTOM_MID, 0, -10);
}

// ===== 数据更新 =====

void display_update(const mpu6050_data_t *data) {
    if (!label_pitch) {
        create_ui();
    }

    // 更新 Pitch
    char buf[32];
    snprintf(buf, sizeof(buf), "Pitch: %+.1f", data->pitch);
    lv_label_set_text(label_pitch, buf);
    lv_bar_set_value(bar_pitch, (int)((data->pitch + 90.0f) * 100.0f / 180.0f), LV_ANIM_OFF);

    // 更新 Roll
    snprintf(buf, sizeof(buf), "Roll: %+.1f", data->roll);
    lv_label_set_text(label_roll, buf);
    lv_bar_set_value(bar_roll, (int)((data->roll + 90.0f) * 100.0f / 180.0f), LV_ANIM_OFF);
}
```

- [ ] **Step 3: 更新 main/CMakeLists.txt**

```cmake
# main/CMakeLists.txt
idf_component_register(SRCS "main.c" "mpu6050.c" "display.c"
                       INCLUDE_DIRS ".")
```

- [ ] **Step 4: 验证编译通过**

```powershell
$env:PATH = "C:\Users\o\.espressif\tools\xtensa-esp-elf\esp-14.2.0_20260121\xtensa-esp-elf\bin;C:\Users\o\.espressif\tools\ninja\1.12.1;C:\Users\o\.espressif\tools\cmake\3.30.5\bin;" + $env:PATH
& "C:\Users\o\.espressif\python_env\idf5.4_py3.13_env\Scripts\python.exe" "D:\ESP-IDF\.espressif\v5.4.4\esp-idf\tools\idf.py" build
```

Expected: BUILD SUCCESSFUL

- [ ] **Step 5: 提交**

```bash
git add main/display.c main/CMakeLists.txt
git commit -m "feat: implement LVGL display module with angle UI"
```

---

### Task 4: 更新主程序集成显示

**Files:**
- Modify: `main/main.c`

- [ ] **Step 1: 更新 main/main.c**

替换整个文件内容：

```c
// main/main.c
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "mpu6050.h"
#include "display.h"

static const char *TAG = "MAIN";

#define I2C_SDA_PIN  21
#define I2C_SCL_PIN  22

void app_main(void) {
    ESP_LOGI(TAG, "=== IMU Gesture Visualizer - Phase 2a ===");

    // 初始化显示（先初始化屏幕，方便看到启动信息）
    esp_err_t ret = display_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize display: %s", esp_err_to_name(ret));
        while (1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
    }
    ESP_LOGI(TAG, "Display initialized");

    // 初始化 I2C
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus_handle;
    ret = i2c_new_master_bus(&bus_config, &bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2C: %s", esp_err_to_name(ret));
        while (1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
    }
    ESP_LOGI(TAG, "I2C initialized: SDA=%d, SCL=%d", I2C_SDA_PIN, I2C_SCL_PIN);

    // 初始化 MPU6050
    mpu6050_data_t mpu_data;
    ret = mpu6050_init(bus_handle, &mpu_data);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize MPU6050: %s", esp_err_to_name(ret));
        while (1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
    }

    ESP_LOGI(TAG, "Starting data read loop...");

    while (1) {
        if (mpu6050_read(&mpu_data) == ESP_OK) {
            display_update(&mpu_data);
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

- [ ] **Step 3: 提交**

```bash
git add main/main.c
git commit -m "feat: integrate display module into main loop"
```

---

### Task 5: 烧录验证

**Files:** 无新增

- [ ] **Step 1: 烧录固件**

```powershell
$env:PATH = "C:\Users\o\.espressif\tools\xtensa-esp-elf\esp-14.2.0_20260121\xtensa-esp-elf\bin;C:\Users\o\.espressif\tools\ninja\1.12.1;C:\Users\o\.espressif\tools\cmake\3.30.5\bin;" + $env:PATH
& "C:\Users\o\.espressif\python_env\idf5.4_py3.13_env\Scripts\python.exe" "D:\ESP-IDF\.espressif\v5.4.4\esp-idf\tools\idf.py" -p COM16 flash monitor
```

- [ ] **Step 2: 验证屏幕显示**

Expected 输出：
1. 屏幕点亮，黑色背景
2. 标题 "IMU Gesture Visualizer" 显示
3. Pitch/Roll 角度数值实时更新
4. 进度条随角度变化
5. 底部状态栏显示 "Mode: FREE | WiFi: --"

- [ ] **Step 3: 按 Ctrl+] 退出监视器**

- [ ] **Step 4: 最终提交**

```bash
git add -A
git commit -m "feat: Phase 2a complete - LVGL display showing MPU6050 angles"
```

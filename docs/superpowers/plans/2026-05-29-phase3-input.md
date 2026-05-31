# Phase 3: 输入设备 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 添加触摸屏和按键输入，实现页面切换功能。

**Architecture:** 新增 touch.c/h 和 input.c/h 模块，实现页面管理器。

**Tech Stack:** ESP-IDF v5.4, XPT2046 触摸, GPIO 按键, LVGL

---

## 硬件接线

### XPT2046 触摸屏（与 LCD 共用 SPI）
| XPT2046 | ESP32 | 说明 |
|---------|-------|------|
| T_IRQ | GPIO 36 | 触摸中断（可选） |
| T_OUT | GPIO 39 | 触摸数据输出 |
| T_DIN | GPIO 32 | 触摸数据输入（与 MPU6050 SDA 冲突，需换引脚） |
| T_CS | GPIO 33 | 触摸片选（与 MPU6050 SCL 冲突，需换引脚） |
| T_CLK | GPIO 18 | 触摸时钟（与 LCD 共用） |

**注意**：T_DIN 和 T_CS 与 MPU6050 的 I2C 引脚冲突，需要重新规划引脚。

### 按键
| 按键 | GPIO | 功能 |
|------|------|------|
| 上键 | GPIO 34 | 上翻/增加 |
| 下键 | GPIO 35 | 下翻/减少 |
| 确认键 | GPIO 13 | 确认/返回 |

---

## 文件变更清单

| 文件 | 操作 | 说明 |
|------|------|------|
| main/touch.c | 新建 | XPT2046 触摸驱动 |
| main/touch.h | 新建 | 触摸头文件 |
| main/input.c | 新建 | 按键输入处理 |
| main/input.h | 新建 | 按键头文件 |
| main/page_manager.c | 新建 | 页面管理器 |
| main/page_manager.h | 新建 | 页面管理头文件 |
| main/main.c | 修改 | 集成输入设备 |
| main/display.c | 修改 | 页面切换逻辑 |
| main/CMakeLists.txt | 修改 | 添加新文件 |

---

### Task 1: 触摸屏驱动

**Files:**
- Create: `main/touch.h`
- Create: `main/touch.c`

- [ ] **Step 1: 创建 touch.h**

```c
#ifndef TOUCH_H
#define TOUCH_H

#include "esp_err.h"

typedef struct {
    int x;
    int y;
    bool pressed;
} touch_event_t;

esp_err_t touch_init(void);
bool touch_read(touch_event_t *event);

#endif
```

- [ ] **Step 2: 创建 touch.c**

XPT2046 触摸驱动，使用 SPI 接口。

**注意**：由于 T_DIN/T_CS 与 MPU6050 冲突，暂时禁用触摸功能，后续重新规划引脚后再启用。

```c
#include "touch.h"
#include "esp_log.h"

static const char *TAG = "TOUCH";

esp_err_t touch_init(void) {
    ESP_LOGW(TAG, "Touch not available - pins conflict with MPU6050");
    return ESP_OK;
}

bool touch_read(touch_event_t *event) {
    event->pressed = false;
    event->x = 0;
    event->y = 0;
    return false;
}
```

- [ ] **Step 3: 提交**

```bash
git add main/touch.c main/touch.h
git commit -m "feat: add touch driver placeholder (pins conflict)"
```

---

### Task 2: 按键输入处理

**Files:**
- Create: `main/input.h`
- Create: `main/input.c`

- [ ] **Step 1: 创建 input.h**

```c
#ifndef INPUT_H
#define INPUT_H

#include "esp_err.h"

typedef enum {
    INPUT_NONE,
    INPUT_UP,
    INPUT_DOWN,
    INPUT_SELECT,
} input_event_t;

esp_err_t input_init(void);
input_event_t input_read(void);

#endif
```

- [ ] **Step 2: 创建 input.c**

按键输入处理，GPIO 34/35/13，带消抖。

```c
#include "input.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "INPUT";

#define BTN_UP_PIN      GPIO_NUM_34
#define BTN_DOWN_PIN    GPIO_NUM_35
#define BTN_SELECT_PIN  GPIO_NUM_13

esp_err_t input_init(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BTN_UP_PIN) | (1ULL << BTN_DOWN_PIN) | (1ULL << BTN_SELECT_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    ESP_LOGI(TAG, "Buttons initialized: UP=%d, DOWN=%d, SELECT=%d", BTN_UP_PIN, BTN_DOWN_PIN, BTN_SELECT_PIN);
    return ESP_OK;
}

input_event_t input_read(void) {
    // 低电平有效（按下接地）
    if (gpio_get_level(BTN_UP_PIN) == 0) {
        vTaskDelay(pdMS_TO_TICKS(50));  // 消抖
        if (gpio_get_level(BTN_UP_PIN) == 0) {
            while (gpio_get_level(BTN_UP_PIN) == 0) vTaskDelay(pdMS_TO_TICKS(10));
            return INPUT_UP;
        }
    }
    if (gpio_get_level(BTN_DOWN_PIN) == 0) {
        vTaskDelay(pdMS_TO_TICKS(50));
        if (gpio_get_level(BTN_DOWN_PIN) == 0) {
            while (gpio_get_level(BTN_DOWN_PIN) == 0) vTaskDelay(pdMS_TO_TICKS(10));
            return INPUT_DOWN;
        }
    }
    if (gpio_get_level(BTN_SELECT_PIN) == 0) {
        vTaskDelay(pdMS_TO_TICKS(50));
        if (gpio_get_level(BTN_SELECT_PIN) == 0) {
            while (gpio_get_level(BTN_SELECT_PIN) == 0) vTaskDelay(pdMS_TO_TICKS(10));
            return INPUT_SELECT;
        }
    }
    return INPUT_NONE;
}
```

- [ ] **Step 3: 提交**

```bash
git add main/input.c main/input.h
git commit -m "feat: add button input handler"
```

---

### Task 3: 页面管理器

**Files:**
- Create: `main/page_manager.h`
- Create: `main/page_manager.c`

- [ ] **Step 1: 创建 page_manager.h**

```c
#ifndef PAGE_MANAGER_H
#define PAGE_MANAGER_H

typedef enum {
    PAGE_HOME,
    PAGE_SPORT,
    PAGE_TOOLS,
    PAGE_GAME,
    PAGE_SETTINGS,
    PAGE_COUNT,
} page_id_t;

void page_manager_init(void);
void page_manager_next(void);
void page_manager_prev(void);
void page_manager_select(void);
page_id_t page_manager_get_current(void);

#endif
```

- [ ] **Step 2: 创建 page_manager.c**

```c
#include "page_manager.h"
#include "display.h"
#include "esp_log.h"

static const char *TAG = "PAGE_MGR";
static page_id_t current_page = PAGE_HOME;

void page_manager_init(void) {
    current_page = PAGE_HOME;
    ESP_LOGI(TAG, "Page manager initialized, current: HOME");
}

void page_manager_next(void) {
    current_page = (current_page + 1) % PAGE_COUNT;
    ESP_LOGI(TAG, "Page changed to: %d", current_page);
}

void page_manager_prev(void) {
    current_page = (current_page + PAGE_COUNT - 1) % PAGE_COUNT;
    ESP_LOGI(TAG, "Page changed to: %d", current_page);
}

void page_manager_select(void) {
    ESP_LOGI(TAG, "Page %d selected", current_page);
}

page_id_t page_manager_get_current(void) {
    return current_page;
}
```

- [ ] **Step 3: 提交**

```bash
git add main/page_manager.c main/page_manager.h
git commit -m "feat: add page manager for screen navigation"
```

---

### Task 4: 集成到主程序

**Files:**
- Modify: `main/main.c`
- Modify: `main/CMakeLists.txt`

- [ ] **Step 1: 更新 CMakeLists.txt**

```cmake
idf_component_register(SRCS "main.c" "mpu6050.c" "display.c" "web_server.c" "dht11.c" "buzzer.c" "input.c" "page_manager.c"
                       INCLUDE_DIRS ".")
```

- [ ] **Step 2: 更新 main.c**

添加按键输入和页面切换逻辑：

```c
#include "input.h"
#include "page_manager.h"

// 在 app_main 中初始化
input_init();
page_manager_init();

// 在主循环中处理按键
while (1) {
    input_event_t event = input_read();
    if (event == INPUT_UP) {
        page_manager_next();
    } else if (event == INPUT_DOWN) {
        page_manager_prev();
    } else if (event == INPUT_SELECT) {
        page_manager_select();
    }
    // ... 其他逻辑
}
```

- [ ] **Step 3: 验证编译**

```powershell
cd D:\STM32CubeFile\IMU-Gesture-Visualizer
$env:PATH = "C:\Users\o\.espressif\tools\xtensa-esp-elf\esp-14.2.0_20260121\xtensa-esp-elf\bin;C:\Users\o\.espressif\tools\ninja\1.12.1;C:\Users\o\.espressif\tools\cmake\3.30.5\bin;" + $env:PATH
& "C:\Users\o\.espressif\python_env\idf5.4_py3.13_env\Scripts\python.exe" "D:\ESP-IDF\.espressif\v5.4.4\esp-idf\tools\idf.py" build
```

- [ ] **Step 4: 烧录测试**

验证：
1. 按键可以切换页面
2. 页面切换时日志输出正确

- [ ] **Step 5: 提交**

```bash
git add -A
git commit -m "feat: Phase 3 complete - input devices and page navigation"
```

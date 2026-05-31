# Phase 6: 扩展功能 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 实现手电筒、摇晃亮屏和贪吃蛇游戏。

**Architecture:** 新增 flashlight.c/h 和 snake.c/h 模块。

**Tech Stack:** ESP-IDF v5.4, MPU6050, LVGL canvas

---

### Task 1: 手电筒模块

**Files:**

- Create: `main/flashlight.h`
- Create: `main/flashlight.c`

- [ ] **Step 1: 创建 flashlight.h**

```c
#ifndef FLASHLIGHT_H
#define FLASHLIGHT_H

void flashlight_init(void);
void flashlight_on(void);
void flashlight_off(void);
void flashlight_toggle(void);
bool flashlight_is_on(void);

#endif
```

- [ ] **Step 2: 创建 flashlight.c**

```c
#include "flashlight.h"
#include "esp_log.h"

static bool state = false;

void flashlight_init(void) {
    state = false;
    ESP_LOGI(TAG, "Flashlight initialized");
}

void flashlight_on(void) {
    state = true;
    ESP_LOGI(TAG, "Flashlight ON");
}

void flashlight_off(void) {
    state = false;
    ESP_LOGI(TAG, "Flashlight OFF");
}

void flashlight_toggle(void) {
    state = !state;
    ESP_LOGI(TAG, "Flashlight %s", state ? "ON" : "OFF");
}

bool flashlight_is_on(void) {
    return state;
}
```

- [ ] **Step 3: 提交**

```bash
git add main/flashlight.c main/flashlight.h
git commit -m "feat: add flashlight module"
```

---

### Task 2: 贪吃蛇游戏模块

**Files:**

- Create: `main/snake.h`
- Create: `main/snake.c`

- [ ] **Step 1: 创建 snake.h**

```c
#ifndef SNAKE_H
#define SNAKE_H

#include "lvgl.h"

void snake_init(lv_obj_t *parent);
void snake_update(void);
void snake_move_up(void);
void snake_move_down(void);
void snake_move_left(void);
void snake_move_right(void);
void snake_reset(void);

#endif
```

- [ ] **Step 2: 创建 snake.c**

```c
#include "snake.h"
#include "esp_log.h"
#include <string.h>

#define GRID_W  15
#define GRID_H  10
#define CELL_SIZE 12

static lv_obj_t *canvas = NULL;
static lv_color_t cbuf[GRID_W * GRID_H];
static int snake_x[50], snake_y[50];
static int snake_len = 3;
static int food_x, food_y;
static int dir_x = 1, dir_y = 0;
static bool game_over = false;

static void draw_cell(int x, int y, lv_color_t color) {
    for (int dy = 0; dy < CELL_SIZE; dy++) {
        for (int dx = 0; dx < CELL_SIZE; dx++) {
            cbuf[(y * CELL_SIZE + dy) * (GRID_W * CELL_SIZE) + (x * CELL_SIZE + dx)] = color;
        }
    }
}

void snake_init(lv_obj_t *parent) {
    canvas = lv_canvas_create(parent);
    lv_canvas_set_buffer(canvas, cbuf, GRID_W * CELL_SIZE, GRID_H * CELL_SIZE, LV_IMG_CF_TRUE_COLOR);
    lv_obj_align(canvas, LV_ALIGN_TOP_MID, 0, 30);
    snake_reset();
}

void snake_reset(void) {
    snake_len = 3;
    for (int i = 0; i < snake_len; i++) {
        snake_x[i] = 5 - i;
        snake_y[i] = 5;
    }
    food_x = 10;
    food_y = 5;
    dir_x = 1;
    dir_y = 0;
    game_over = false;
}

void snake_update(void) {
    if (game_over) return;

    // 移动
    for (int i = snake_len - 1; i > 0; i--) {
        snake_x[i] = snake_x[i-1];
        snake_y[i] = snake_y[i-1];
    }
    snake_x[0] += dir_x;
    snake_y[0] += dir_y;

    // 碰撞检测
    if (snake_x[0] < 0 || snake_x[0] >= GRID_W ||
        snake_y[0] < 0 || snake_y[0] >= GRID_H) {
        game_over = true;
        return;
    }

    // 吃食物
    if (snake_x[0] == food_x && snake_y[0] == food_y) {
        snake_len++;
        food_x = esp_random() % GRID_W;
        food_y = esp_random() % GRID_H;
    }

    // 绘制
    memset(cbuf, 0, sizeof(cbuf));
    for (int i = 0; i < snake_len; i++) {
        draw_cell(snake_x[i], snake_y[i], lv_color_make(0, 255, 0));
    }
    draw_cell(food_x, food_y, lv_color_make(255, 0, 0));
    lv_obj_invalidate(canvas);
}

void snake_move_up(void) { if (dir_y != 1) { dir_x = 0; dir_y = -1; } }
void snake_move_down(void) { if (dir_y != -1) { dir_x = 0; dir_y = 1; } }
void snake_move_left(void) { if (dir_x != 1) { dir_x = -1; dir_y = 0; } }
void snake_move_right(void) { if (dir_x != -1) { dir_x = 1; dir_y = 0; } }
```

- [ ] **Step 3: 提交**

```bash
git add main/snake.c main/snake.h
git commit -m "feat: add snake game module"
```

---

### Task 3: 集成到主程序

**Files:**

- Modify: `main/main.c`
- Modify: `main/CMakeLists.txt`

- [ ] **Step 1: 更新 CMakeLists.txt**

```cmake
idf_component_register(SRCS "main.c" "mpu6050.c" "display.c" "web_server.c" "dht11.c" "buzzer.c"
                       "touch.c" "input.c" "page_manager.c" "pedometer.c" "motion_detect.c"
                       "stopwatch.c" "countdown.c" "flashlight.c" "snake.c"
                       INCLUDE_DIRS ".")
```

- [ ] **Step 2: 更新 main.c**

添加手电筒和贪吃蛇集成：

```c
#include "flashlight.h"
#include "snake.h"

// 在按键处理中
if (page_manager_get_current() == PAGE_TOOLS) {
    if (event == INPUT_SELECT) {
        flashlight_toggle();
    }
}

if (page_manager_get_current() == PAGE_GAME) {
    if (event == INPUT_UP) snake_move_up();
    else if (event == INPUT_DOWN) snake_move_down();
    else if (event == INPUT_LEFT) snake_move_left();
    else if (event == INPUT_RIGHT) snake_move_right();
}
```

- [ ] **Step 3: 验证编译**

```powershell
cd D:\STM32CubeFile\IMU-Gesture-Visualizer
$env:PATH = "C:\Users\o\.espressif\tools\xtensa-esp-elf\esp-14.2.0_20260121\xtensa-esp-elf\bin;C:\Users\o\.espressif\tools\ninja\1.12.1;C:\Users\o\.espressif\tools\cmake\3.30.5\bin;" + $env:PATH
& "C:\Users\o\.espressif\python_env\idf5.4_py3.13_env\Scripts\python.exe" "D:\ESP-IDF\.espressif\v5.4.4\esp-idf\tools\idf.py" build
```

- [ ] **Step 4: 提交**

```bash
git add -A
git commit -m "feat: Phase 6 complete - extended features (flashlight, snake)"
```

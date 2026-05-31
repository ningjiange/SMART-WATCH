# Phase 2: LVGL 显示设计文档

## 目标

在 ILI9341 TFT 屏幕上显示 MPU6050 姿态数据，包括角度数值、进度条和底部状态栏。

## 硬件接线

| ILI9341 引脚 | ESP32 引脚 | 说明 |
|-------------|-----------|------|
| VCC | 3.3V | 电源 |
| GND | GND | 地 |
| CS | GPIO 15 | 片选 |
| RESET | GPIO 4 | 复位 |
| D/C | GPIO 2 | 数据/命令选择 |
| SDI(MOSI) | GPIO 23 | SPI 主出从入 |
| SCK | GPIO 18 | SPI 时钟 |
| LED | 3.3V | 背光常亮 |
| SDO(MISO) | GPIO 19 | SPI 主入从出（可选） |
| 触摸引脚 | 不接 | Phase 2 不用触摸功能 |

## LVGL 集成

使用 ESP-IDF 组件管理器，在 `main/idf_component.yml` 添加依赖：

```yaml
dependencies:
  lvgl: "~8.4"
  esp_lvgl_port: "~2.4"
  esp_lcd_ili9341: "~1.1"
  esp_lcd_touch_tt2046: "~1.1"  # 触摸驱动备用
```

## 界面布局

```
┌─────────────────────────────────┐
│  IMU Gesture Visualizer        │  ← 标题栏
├─────────────────────────────────┤
│                                 │
│  Pitch: 12.5°                   │  ← 俯仰角数值
│  [████████████░░░░] 50%         │  ← 俯仰角进度条
│                                 │
│  Roll: -5.2°                    │  ← 横滚角数值
│  [████░░░░░░░░░░░░] 25%         │  ← 横滚角进度条
│                                 │
├─────────────────────────────────┤
│ Yaw: 0° | Mode: FREE | WiFi: -- │  ← 底部状态栏
└─────────────────────────────────┘
```

## 文件结构

```
main/
├── main.c              — 入口：I2C + LVGL 初始化 + 主循环
├── mpu6050.c           — MPU6050 驱动（已有）
├── mpu6050.h           — MPU6050 头文件（已有）
├── display.h           — 显示模块头文件
├── display.c           — 显示模块：LVGL 初始化 + UI 创建
├── CMakeLists.txt      — 组件 CMake（需更新）
└── idf_component.yml   — LVGL 依赖声明
```

## 模块设计

### display.h / display.c

```c
// display.h
#ifndef DISPLAY_H
#define DISPLAY_H

#include "mpu6050.h"

// 初始化 LVGL 和屏幕
esp_err_t display_init(void);

// 更新显示数据
void display_update(const mpu6050_data_t *data);

#endif
```

主要函数：
- `display_init()` — 初始化 LVGL、ILI9341 驱动、创建 UI
- `display_update()` — 更新角度数值和进度条

### UI 组件

1. **标题栏** — `lv_label` 显示项目名称
2. **角度数值** — `lv_label` 显示 Pitch/Roll 数值
3. **进度条** — `lv_bar` 显示角度百分比（-90°~+90° 映射到 0~100）
4. **状态栏** — `lv_label` 显示 Yaw、模式、连接状态

### 数据流

```
MPU6050 → mpu6050_data_t → display_update() → LVGL 刷新
```

主循环每 100ms 调用一次 `display_update()`。

## 颜色方案

| 元素 | 颜色 | LVGL 宏 |
|------|------|---------|
| 背景 | 黑色 | `lv_color_black()` |
| 标题 | 白色 | `lv_color_white()` |
| Pitch 数值 | 绿色 | `lv_color_make(0, 255, 0)` |
| Roll 数值 | 蓝色 | `lv_color_make(0, 128, 255)` |
| 进度条背景 | 深灰 | `lv_color_make(50, 50, 50)` |
| 进度条填充 | 对应颜色 | 绿色/蓝色 |
| 状态栏 | 灰色 | `lv_color_make(128, 128, 128)` |

## 依赖组件

| 组件 | 版本 | 用途 |
|------|------|------|
| lvgl | ~8.4 | GUI 核心库 |
| esp_lvgl_port | ~2.4 | ESP-IDF LVGL 移植层 |
| esp_lcd_ili9341 | ~1.1 | ILI9341 驱动 |

## 验证标准

1. 屏幕点亮，显示黑色背景
2. 标题 "IMU Gesture Visualizer" 显示正常
3. Pitch/Roll 角度数值实时更新
4. 进度条随角度变化
5. 底部状态栏显示正常
6. 倾斜板子时，数值和进度条同步变化

## Phase 2b 预留

Phase 2b 将添加：
- 3D 立方体绘制（lv_canvas）
- 立方体随 MPU6050 数据旋转
- 互补滤波姿态解算

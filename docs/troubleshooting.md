# ESP32 智能手表 — 问题排查与解决方案

## 项目概述

ESP32 + MPU6050 + ILI9341 TFT 屏幕 + WiFi Web Server 的智能手表项目。

---

## 核心问题总结

### 1. ESP-IDF 环境配置

**问题**：`idf.py` 命令找不到，或 MSys/Mingw 环境不兼容

**解决方案**：
- 在 Windows 原生 CMD 或 PowerShell 中运行
- 先安装工具链：`python.exe idf_tools.py install`
- 激活环境：`D:\ESP-IDF\.espressif\v5.4.4\esp-idf\export.bat`
- PowerShell 中需要手动设置 PATH：
```powershell
$env:PATH = "C:\Users\o\.espressif\tools\xtensa-esp-elf\esp-14.2.0_20260121\xtensa-esp-elf\bin;C:\Users\o\.espressif\tools\ninja\1.12.1;C:\Users\o\.espressif\tools\cmake\3.30.5\bin;" + $env:PATH
```

---

### 2. ILI9341 屏幕初始化

**问题**：屏幕显示颜色不对、方向颠倒、只显示部分区域

**解决方案**：

**颜色格式**：ILI9341 需要大端字节序（高字节在前）
```c
// 正确：大端
buf[0] = color >> 8;   // 高字节
buf[1] = color & 0xFF; // 低字节

// 错误：小端（会导致颜色混乱）
buf[0] = color & 0xFF;
buf[1] = color >> 8;
```

**屏幕方向**：通过 0x36 寄存器（MADCTL）控制
| 值 | 方向 |
|----|------|
| 0x48 | 竖屏，引脚在下 |
| 0x88 | 竖屏，引脚在上（翻转180°） |
| 0x28 | 横屏 |
| 0x68 | 横屏（翻转180°） |

位定义：
- Bit 6 (0x40): MY - 行地址顺序
- Bit 5 (0x20): MX - 列地址顺序  
- Bit 4 (0x10): MV - 行列交换
- Bit 3 (0x08): BGR - RGB/BGR 顺序

**简化初始化**：只用必要命令即可工作
```c
lcd_cmd(0x01);  // Software reset
vTaskDelay(pdMS_TO_TICKS(150));
lcd_cmd(0x11);  // Sleep out
vTaskDelay(pdMS_TO_TICKS(120));
lcd_cmd(0x36);  // MADCTL
lcd_data(0x88); // 方向
lcd_cmd(0x3A);  // Pixel format
lcd_data(0x55); // 16bit
lcd_cmd(0x29);  // Display on
vTaskDelay(pdMS_TO_TICKS(50));
```

---

### 3. LVGL API 版本兼容

**问题**：LVGL 8.4 和 9.x API 完全不同，编译报错

**解决方案**：

| LVGL 8.4 (正确) | LVGL 9.x (错误) |
|-----------------|-----------------|
| `lv_disp_drv_t` | `lv_display_t` |
| `lv_disp_flush_ready()` | `lv_display_flush_ready()` |
| `lv_disp_drv_init()` | `lv_display_create()` |
| `lv_canvas_set_buffer(..., LV_IMG_CF_TRUE_COLOR)` | `lv_canvas_set_buffer(..., LV_COLOR_FORMAT_RGB565)` |
| `lv_font_montserrat_14` | 默认只有 14px |

**关键配置**：`sdkconfig.defaults` 中确保：
```
CONFIG_LV_COLOR_DEPTH_16=y
```

---

### 4. WiFi + I2C 冲突（核心难点）

**问题**：同时使用 WiFi 和硬件 I2C 时，系统频繁崩溃重启

**错误表现**：
```
Guru Meditation Error: Core 0 panic'ed (Interrupt wdt timeout)
i2c.master: I2C software timeout
i2c.common: GPIO XX is not usable, maybe conflict with others
```

**根本原因**：ESP32 的 WiFi ISR 和 I2C ISR 在同一个 Core (Core 0) 上竞争，导致死锁

**解决方案**：

1. **MPU6050 任务固定到 Core 1**：
```c
xTaskCreatePinnedToCore(mpu_task, "mpu_task", 4096, bus_handle, 5, NULL, 1);
```

2. **禁用 MPU6050 任务的看门狗**：
```c
esp_task_wdt_delete(NULL);
```

3. **降低 I2C 速度到 100kHz**：
```c
.scl_speed_hz = 100000,
```

4. **I2C 超时设为 200ms**：
```c
i2c_master_transmit_receive(dev_handle, &reg, 1, data, len, 200);
```

5. **WiFi 任务低优先级 + 大栈**：
```c
xTaskCreate(wifi_task, "wifi_task", 8192, NULL, 2, NULL);
```

6. **用互斥锁保护共享数据**：
```c
static SemaphoreHandle_t g_data_mutex;
// 读写 mpu_data 时加锁
```

---

### 5. MPU6050 芯片识别

**问题**：WHO_AM_I 返回 0x70 而不是 0x68

**解决方案**：很多标称 MPU6050 的模块实际是 MPU6886，两者的 WHO_AM_I 不同：
- MPU6050: 0x68
- MPU6886: 0x70

修改代码同时支持两种：
```c
if (who_am_i != 0x68 && who_am_i != 0x70) {
    ESP_LOGE(TAG, "Invalid device! Got 0x%02X", who_am_i);
    return ESP_ERR_INVALID_RESPONSE;
}
```

---

### 6. 任务栈溢出

**问题**：任务栈太小导致崩溃

**经验值**：
- LVGL 任务：默认 4096 足够
- WiFi 任务：至少 8192
- I2C/MPU6050 任务：4096 足够

---

### 7. LVGL 白屏问题

**问题**：屏幕初始化成功但显示白屏

**解决方案**：
1. 使用 `esp_lvgl_port` 而不是原生 LVGL API
2. 确保 LVGL 任务正确启动
3. 检查显示缓冲区大小

```c
// 正确：使用 esp_lvgl_port
const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

// 错误：原生 API 可能导致白屏
lv_init();
```

---

### 8. 按键 GPIO 问题

**问题**：GPIO 34/35 按键不响应

**根本原因**：GPIO 34/35 是输入专用引脚，没有内部上拉电阻

**解决方案**：
1. 外接上拉电阻（10KΩ 到 3.3V）
2. 或改用有内部上拉的 GPIO（如 12, 14, 27）

```c
// 推荐使用的 GPIO（有内部上拉）
#define GPIO_INPUT_UP      12
#define GPIO_INPUT_DOWN    14
#define GPIO_INPUT_SELECT  27
```

---

### 9. DHT11 读取不稳定

**问题**：DHT11 数据跳动、校验错误

**解决方案**：
1. 使用 `ets_delay_us()` 代替 `esp_rom_delay_us()`
2. 添加数据过滤，只返回有效值
3. 错误时返回上一次的有效数据

```c
#include "rom/ets_sys.h"

// 数据有效性检查
if (temp < -20 || temp > 60 || humi < 0 || humi > 100) {
    *temperature = last_temp;
    *humidity = last_humi;
    return ESP_OK;
}
```

---

### 10. 按键响应不灵敏

**问题**：按键按下后反应慢或无反应

**根本原因**：
1. 按键检测在主循环中被其他任务阻塞
2. 消抖等待时间太长

**解决方案**：
1. 将按键检测放到独立的高优先级任务
2. 减少消抖时间（20ms）
3. 按键触发后添加防抖等待

```c
// 独立按键任务（优先级 6）
static void button_task(void *pvParameters) {
    while (1) {
        input_event_t event = input_read();
        if (event != INPUT_NONE) {
            // 处理按键
            vTaskDelay(pdMS_TO_TICKS(300));  // 防抖等待
        }
        vTaskDelay(pdMS_TO_TICKS(10));  // 10ms 检测一次
    }
}
```

---

## ESP32 引脚使用情况

| 引脚 | 用途 | 备注 |
|------|------|------|
| GPIO 2 | LCD DC | 数据/命令选择 |
| GPIO 4 | LCD RESET | 复位 |
| GPIO 12 | 按键上翻 | 有内部上拉 |
| GPIO 14 | 按键下翻 | 有内部上拉 |
| GPIO 15 | LCD CS | 片选 |
| GPIO 18 | LCD SCK | SPI 时钟 |
| GPIO 19 | LCD MISO | SPI 主入（可选） |
| GPIO 23 | LCD MOSI | SPI 主出 |
| GPIO 25 | DHT11 | 温湿度传感器 |
| GPIO 26 | 蜂鸣器 | 低电平有效 |
| GPIO 27 | 按键确认 | 有内部上拉 |
| GPIO 32 | MPU6050 SDA | I2C 数据 |
| GPIO 33 | MPU6050 SCL | I2C 时钟 |

**注意**：
- GPIO 21/22 在某些配置下会被占用，避免使用
- GPIO 34/35 是输入专用，无内部上拉，需外接电阻

---

## 调试技巧

1. **查看内存使用**：`idf.py size-components`
2. **查看分区表**：检查 app 分区是否足够大
3. **串口监视器**：`idf.py -p COMx monitor`
4. **按 Ctrl+] 退出监视器**

---

## 文件修改记录

| 日期 | 修改 | 原因 |
|------|------|------|
| 2026-05-28 | 创建项目 | Phase 1: MPU6050 串口验证 |
| 2026-05-28 | 修改 mpu6050.c | 支持 MPU6886 (WHO_AM_I = 0x70) |
| 2026-05-28 | 修复 .vscode/settings.json | 更新中文路径到英文路径 |
| 2026-05-29 | 添加 LVGL + ILI9341 驱动 | Phase 2a: 屏幕显示角度 |
| 2026-05-29 | 添加 3D 立方体 | Phase 2b: 可视化 |
| 2026-05-29 | 添加 WiFi Web Server | Phase 3: 远程查看 |
| 2026-05-29 | 修复 WiFi+I2C 冲突 | MPU6050 固定到 Core 1 |
| 2026-05-29 | 移除 3D 立方体 | Phase 1: 重构为智能手表 |
| 2026-05-29 | 添加 DHT11 + 蜂鸣器 | Phase 2: 传感器集成 |
| 2026-05-29 | 添加按键 + 页面管理 | Phase 3: 输入设备 |
| 2026-05-29 | 修复按键 GPIO | GPIO 34/35 改为 12/14/27 |
| 2026-05-29 | 修复按键响应 | 独立任务 + 防抖 |

# IMU Gesture Visualizer — 问题排查与解决方案

## 项目概述

ESP32 + MPU6050 + ILI9341 TFT 屏幕 + WiFi Web Server 的 IMU 手势可视化项目。

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

## ESP32 引脚使用情况

| 引脚 | 用途 | 备注 |
|------|------|------|
| GPIO 2 | LCD DC | 数据/命令选择 |
| GPIO 4 | LCD RESET | 复位 |
| GPIO 15 | LCD CS | 片选 |
| GPIO 18 | LCD SCK | SPI 时钟 |
| GPIO 19 | LCD MISO | SPI 主入（可选） |
| GPIO 23 | LCD MOSI | SPI 主出 |
| GPIO 32 | MPU6050 SDA | I2C 数据 |
| GPIO 33 | MPU6050 SCL | I2C 时钟 |

**注意**：GPIO 21/22 在某些配置下会被占用，避免使用

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

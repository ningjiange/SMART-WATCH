# Phase 1: MPU6050 串口验证 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 验证 ESP32 与 MPU6050 的 I2C 通信，通过串口打印加速度计和陀螺仪数据。

**Architecture:** 使用 ESP-IDF 的 `i2c_master` API 驱动 MPU6050，手写寄存器读写函数。主循环每 100ms 读取一次传感器数据，计算 Pitch/Roll 角度后通过串口输出。

**Tech Stack:** ESP-IDF v5.x, I2C HAL, C语言

---

## 文件结构

```
IMU手势可视化/
├── main/
│   ├── main.c              — 入口：I2C 初始化 + MPU6050 读取循环
│   ├── mpu6050.c           — MPU6050 驱动：I2C 寄存器读写 + 数据解析
│   ├── mpu6050.h           — MPU6050 头文件：寄存器定义 + 数据结构 + API
│   └── CMakeLists.txt      — main 组件 CMake
├── CMakeLists.txt          — 项目级 CMake
└── sdkconfig.defaults      — ESP-IDF 默认配置
```

---

### Task 1: 创建 ESP-IDF 项目骨架

**Files:**
- Create: `CMakeLists.txt`
- Create: `main/CMakeLists.txt`
- Create: `sdkconfig.defaults`
- Create: `main/main.c` (minimal placeholder)

- [ ] **Step 1: 创建项目级 CMakeLists.txt**

```cmake
# IMU手势可视化/CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(imu_gesture_visualizer)
```

- [ ] **Step 2: 创建 main 组件 CMakeLists.txt**

```cmake
# IMU手势可视化/main/CMakeLists.txt
idf_component_register(SRCS "main.c" "mpu6050.c"
                       INCLUDE_DIRS ".")
```

- [ ] **Step 3: 创建 sdkconfig.defaults**

```
# IMU手势可视化/sdkconfig.defaults
# 串口波特率
CONFIG_ESP_CONSOLE_UART_DEFAULT=y
CONFIG_ESP_CONSOLE_UART_BAUDRATE=115200

# I2C 配置
CONFIG_I2C_ENABLE_DEBUG_LOG=n
```

- [ ] **Step 4: 创建 main/mpu6050.h（占位）**

```c
// IMU手势可视化/main/mpu6050.h
#ifndef MPU6050_H
#define MPU6050_H

#include "driver/i2c_master.h"

// MPU6050 I2C 地址（AD0 接 GND 时为 0x68）
#define MPU6050_ADDR 0x68

// 寄存器地址
#define MPU6050_REG_WHO_AM_I     0x75
#define MPU6050_REG_PWR_MGMT_1   0x6B
#define MPU6050_REG_SMPLRT_DIV   0x19
#define MPU6050_REG_CONFIG       0x1A
#define MPU6050_REG_ACCEL_CONFIG 0x1C
#define MPU6050_REG_GYRO_CONFIG  0x1B
#define MPU6050_REG_ACCEL_XOUT_H 0x3B
#define MPU6050_REG_GYRO_XOUT_H  0x43

// 姿态数据结构
typedef struct {
    float accel_x;    // 加速度计 X 轴 (m/s²)
    float accel_y;    // 加速度计 Y 轴 (m/s²)
    float accel_z;    // 加速度计 Z 轴 (m/s²)
    float gyro_x;     // 陀螺仪 X 轴 (°/s)
    float gyro_y;     // 陀螺仪 Y 轴 (°/s)
    float gyro_z;     // 陀螺仪 Z 轴 (°/s)
    float pitch;      // 俯仰角 (°)
    float roll;       // 横滚角 (°)
} mpu6050_data_t;

// API 函数
esp_err_t mpu6050_init(i2c_master_bus_handle_t bus_handle, mpu6050_data_t *data);
esp_err_t mpu6050_read(mpu6050_data_t *data);

#endif
```

- [ ] **Step 5: 创建 main/mpu6050.c（占位）**

```c
// IMU手势可视化/main/mpu6050.c
#include "mpu6050.h"
#include "esp_log.h"

static const char *TAG = "MPU6050";
static i2c_master_dev_handle_t dev_handle = NULL;

esp_err_t mpu6050_init(i2c_master_bus_handle_t bus_handle, mpu6050_data_t *data) {
    ESP_LOGI(TAG, "Initializing MPU6050...");
    return ESP_OK;
}

esp_err_t mpu6050_read(mpu6050_data_t *data) {
    return ESP_OK;
}
```

- [ ] **Step 6: 创建 main/main.c（占位）**

```c
// IMU手势可视化/main/main.c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "mpu6050.h"

static const char *TAG = "MAIN";

void app_main(void) {
    ESP_LOGI(TAG, "IMU Gesture Visualizer - Phase 1");
    ESP_LOGI(TAG, "MPU6050 Serial Verification");

    // TODO: 后续步骤实现
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

- [ ] **Step 7: 验证项目能编译通过**

Run: `cd "D:/STM32CubeFile/IMU手势可视化" && idf.py set-target esp32 && idf.py build`
Expected: BUILD SUCCESSFUL

- [ ] **Step 8: 提交**

```bash
git add CMakeLists.txt main/ sdkconfig.defaults
git commit -m "feat: create ESP-IDF project skeleton for MPU6050 verification"
```

---

### Task 2: 实现 MPU6050 I2C 驱动

**Files:**
- Modify: `main/mpu6050.h` — 添加 I2C 句柄存储
- Modify: `main/mpu6050.c` — 实现完整的 I2C 读写 + 初始化 + 数据读取

- [ ] **Step 1: 更新 main/mpu6050.h**

在 `mpu6050_data_t` 结构体定义之前，添加设备句柄声明。将完整文件替换为：

```c
// IMU手势可视化/main/mpu6050.h
#ifndef MPU6050_H
#define MPU6050_H

#include "driver/i2c_master.h"
#include "esp_err.h"

// MPU6050 I2C 地址（AD0 接 GND 时为 0x68）
#define MPU6050_ADDR 0x68

// 寄存器地址
#define MPU6050_REG_WHO_AM_I     0x75
#define MPU6050_REG_PWR_MGMT_1   0x6B
#define MPU6050_REG_SMPLRT_DIV   0x19
#define MPU6050_REG_CONFIG       0x1C
#define MPU6050_REG_ACCEL_CONFIG 0x1C
#define MPU6050_REG_GYRO_CONFIG  0x1B
#define MPU6050_REG_ACCEL_XOUT_H 0x3B
#define MPU6050_REG_GYRO_XOUT_H  0x43

// 姿态数据结构
typedef struct {
    float accel_x;    // 加速度计 X 轴 (m/s²)
    float accel_y;    // 加速度计 Y 轴 (m/s²)
    float accel_z;    // 加速度计 Z 轴 (m/s²)
    float gyro_x;     // 陀螺仪 X 轴 (°/s)
    float gyro_y;     // 陀螺仪 Y 轴 (°/s)
    float gyro_z;     // 陀螺仪 Z 轴 (°/s)
    float pitch;      // 俯仰角 (°)
    float roll;       // 横滚角 (°)
} mpu6050_data_t;

/**
 * @brief 初始化 MPU6050
 * @param bus_handle I2C 总线句柄
 * @param data 输出数据结构体
 * @return ESP_OK 成功
 */
esp_err_t mpu6050_init(i2c_master_bus_handle_t bus_handle, mpu6050_data_t *data);

/**
 * @brief 读取 MPU6050 数据并计算角度
 * @param data 输出数据结构体
 * @return ESP_OK 成功
 */
esp_err_t mpu6050_read(mpu6050_data_t *data);

#endif
```

- [ ] **Step 2: 实现完整的 mpu6050.c**

将整个文件替换为：

```c
// IMU手势可视化/main/mpu6050.c
#include "mpu6050.h"
#include "esp_log.h"
#include <math.h>

static const char *TAG = "MPU6050";
static i2c_master_dev_handle_t dev_handle = NULL;

// I2C 写单个寄存器
static esp_err_t mpu6050_write_reg(uint8_t reg, uint8_t value) {
    return i2c_master_transmit(dev_handle, &reg, 1, 100);
}

// I2C 读多个字节
static esp_err_t mpu6050_read_regs(uint8_t reg, uint8_t *data, size_t len) {
    return i2c_master_transmit_receive(dev_handle, &reg, 1, data, len, 100);
}

// 读取单个寄存器
static esp_err_t mpu6050_read_reg(uint8_t reg, uint8_t *value) {
    return mpu6050_read_regs(reg, value, 1);
}

// 将两个字节组合为有符号 16 位整数
static int16_t combine_bytes(uint8_t high, uint8_t low) {
    return (int16_t)((high << 8) | low);
}

esp_err_t mpu6050_init(i2c_master_bus_handle_t bus_handle, mpu6050_data_t *data) {
    ESP_LOGI(TAG, "Initializing MPU6050...");

    // 创建 I2C 设备
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = MPU6050_ADDR,
        .scl_speed_hz = 400000,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

    // 检测 WHO_AM_I
    uint8_t who_am_i = 0;
    esp_err_t ret = mpu6050_read_reg(MPU6050_REG_WHO_AM_I, &who_am_i);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read WHO_AM_I: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "WHO_AM_I = 0x%02X (expected 0x68)", who_am_i);
    if (who_am_i != 0x68) {
        ESP_LOGE(TAG, "Invalid MPU6050 device! Got 0x%02X", who_am_i);
        return ESP_ERR_INVALID_response;
    }

    // 唤醒 MPU6050（清除 sleep 位）
    ESP_ERROR_CHECK(mpu6050_write_reg(MPU6050_REG_PWR_MGMT_1, 0x00));
    vTaskDelay(pdMS_TO_TICKS(100));

    // 配置采样率 1kHz / (1+9) = 100Hz
    ESP_ERROR_CHECK(mpu6050_write_reg(MPU6050_REG_SMPLRT_DIV, 0x09));

    // 配置低通滤波 DLPF 带宽 44Hz
    ESP_ERROR_CHECK(mpu6050_write_reg(MPU6050_REG_CONFIG, 0x03));

    // 配置加速度计量程 ±2g
    ESP_ERROR_CHECK(mpu6050_write_reg(MPU6050_REG_ACCEL_CONFIG, 0x00));

    // 配置陀螺仪量程 ±250°/s
    ESP_ERROR_CHECK(mpu6050_write_reg(MPU6050_REG_GYRO_CONFIG, 0x00));

    ESP_LOGI(TAG, "MPU6050 initialized successfully");
    return ESP_OK;
}

esp_err_t mpu6050_read(mpu6050_data_t *data) {
    uint8_t buf[14];
    esp_err_t ret = mpu6050_read_regs(MPU6050_REG_ACCEL_XOUT_H, buf, 14);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read sensor data: %s", esp_err_to_name(ret));
        return ret;
    }

    // 解析加速度计数据（原始值 → m/s²）
    int16_t raw_ax = combine_bytes(buf[0], buf[1]);
    int16_t raw_ay = combine_bytes(buf[2], buf[3]);
    int16_t raw_az = combine_bytes(buf[4], buf[5]);
    data->accel_x = raw_ax * 9.81f / 16384.0f;
    data->accel_y = raw_ay * 9.81f / 16384.0f;
    data->accel_z = raw_az * 9.81f / 16384.0f;

    // 解析温度（跳过 buf[6], buf[7]）

    // 解析陀螺仪数据（原始值 → °/s）
    int16_t raw_gx = combine_bytes(buf[8], buf[9]);
    int16_t raw_gy = combine_bytes(buf[10], buf[11]);
    int16_t raw_gz = combine_bytes(buf[12], buf[13]);
    data->gyro_x = raw_gx * 250.0f / 32768.0f;
    data->gyro_y = raw_gy * 250.0f / 32768.0f;
    data->gyro_z = raw_gz * 250.0f / 32768.0f;

    // 从加速度计计算 Pitch 和 Roll
    data->pitch = atan2f(data->accel_x, sqrtf(data->accel_y * data->accel_y + data->accel_z * data->accel_z)) * 180.0f / M_PI;
    data->roll = atan2f(data->accel_y, sqrtf(data->accel_x * data->accel_x + data->accel_z * data->accel_z)) * 180.0f / M_PI;

    return ESP_OK;
}
```

- [ ] **Step 3: 验证编译通过**

Run: `cd "D:/STM32CubeFile/IMU手势可视化" && idf.py build`
Expected: BUILD SUCCESSFUL

- [ ] **Step 4: 提交**

```bash
git add main/mpu6050.h main/mpu6050.c
git commit -m "feat: implement MPU6050 I2C driver with angle calculation"
```

---

### Task 3: 实现主程序入口

**Files:**
- Modify: `main/main.c` — I2C 初始化 + MPU6050 读取循环 + 串口输出

- [ ] **Step 1: 更新 main/main.c**

将整个文件替换为：

```c
// IMU手势可视化/main/main.c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "mpu6050.h"

static const char *TAG = "MAIN";

// I2C 配置
#define I2C_SDA_PIN  21
#define I2C_SCL_PIN  22
#define I2C_FREQ_HZ  400000

void app_main(void) {
    ESP_LOGI(TAG, "=== IMU Gesture Visualizer - Phase 1 ===");
    ESP_LOGI(TAG, "MPU6050 Serial Verification");

    // 初始化 I2C 总线
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));
    ESP_LOGI(TAG, "I2C initialized: SDA=%d, SCL=%d", I2C_SDA_PIN, I2C_SCL_PIN);

    // 初始化 MPU6050
    mpu6050_data_t mpu_data;
    ESP_ERROR_CHECK(mpu6050_init(bus_handle, &mpu_data));

    ESP_LOGI(TAG, "Starting data read loop...");
    ESP_LOGI(TAG, "%-8s %-8s %-8s | %-8s %-8s %-8s | %-8s %-8s",
             "Accel_X", "Accel_Y", "Accel_Z", "Gyro_X", "Gyro_Y", "Gyro_Z", "Pitch", "Roll");
    ESP_LOGI(TAG, "-------- -------- -------- | -------- -------- -------- | -------- --------");

    while (1) {
        if (mpu6050_read(&mpu_data) == ESP_OK) {
            ESP_LOGI(TAG, "%7.2f  %7.2f  %7.2f | %7.2f  %7.2f  %7.2f | %6.1f°  %6.1f°",
                     mpu_data.accel_x, mpu_data.accel_y, mpu_data.accel_z,
                     mpu_data.gyro_x, mpu_data.gyro_y, mpu_data.gyro_z,
                     mpu_data.pitch, mpu_data.roll);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
```

- [ ] **Step 2: 验证编译通过**

Run: `cd "D:/STM32CubeFile/IMU手势可视化" && idf.py build`
Expected: BUILD SUCCESSFUL

- [ ] **Step 3: 提交**

```bash
git add main/main.c
git commit -m "feat: implement main loop with MPU6050 data read and serial output"
```

---

### Task 4: 烧录并验证

**Files:** 无新增文件

- [ ] **Step 1: 确认 ESP32 已连接**

Run: `cd "D:/STM32CubeFile/IMU手势可视化" && idf.py monitor --port COMx`

注意：需要先确认 ESP32 的 COM 端口号。在设备管理器中查看"端口(COM 和 LPT)"下的 CH340 COM 端口。

- [ ] **Step 2: 烧录固件**

Run: `cd "D:/STM32CubeFile/IMU手势可视化" && idf.py -p COMx flash`

Expected: 烧录成功，自动重启

- [ ] **Step 3: 打开串口监视器**

Run: `cd "D:/STM32CubeFile/IMU手势可视化" && idf.py -p COMx monitor`

Expected 输出：
```
I (xxx) MAIN: === IMU Gesture Visualizer - Phase 1 ===
I (xxx) MAIN: MPU6050 Serial Verification
I (xxx) MAIN: I2C initialized: SDA=21, SCL=22
I (xxx) MPU6050: Initializing MPU6050...
I (xxx) MPU6050: WHO_AM_I = 0x68 (expected 0x68)
I (xxx) MPU6050: MPU6050 initialized successfully
I (xxx) MAIN: Starting data read loop...
I (xxx) MAIN: Accel_X  Accel_Y  Accel_Z  | Gyro_X   Gyro_Y   Gyro_Z  | Pitch    Roll
I (xxx) MAIN: -------- -------- -------- | -------- -------- -------- | -------- --------
I (xxx) MAIN:    0.12    -0.03     9.81  |    0.50    -1.20     0.30  |   0.7°   -1.2°
```

- [ ] **Step 4: 验证数据变化**

倾斜 ESP32 开发板，观察 Pitch 和 Roll 角度是否随之变化。如果角度随板子倾斜而变化，说明 MPU6050 工作正常。

- [ ] **Step 5: 按 Ctrl+] 退出监视器**

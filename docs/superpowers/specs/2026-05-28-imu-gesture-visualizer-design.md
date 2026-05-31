# IMU Gesture Visualizer — 设计文档

## 项目概述

ESP32 读取 MPU6050 姿态数据，LVGL 屏幕实时显示 3D 立方体旋转 + 角度图表，WiFi 提供网页实时查看，BLE 支持手机指令控制。

**目标**：放简历找实习（嵌入式/IoT 方向），与现有"智能环境监测站"项目互补。

**复杂度**：轻量级，预计 1~2 周完成。

## 硬件清单

| 模块                            | 用途         | 接口 | 来源 |
| ------------------------------- | ------------ | ---- | ---- |
| ESP32-WROOM-32                  | 主控         | —    | 新买 |
| 2.4寸 SPI 屏 (ILI9341, 320x240) | LVGL 显示    | SPI  | 新买 |
| MPU6050                         | 姿态传感器   | I2C  | 复用 |
| W25Q64 Flash                    | 历史数据存储 | SPI  | 复用 |
| 蜂鸣器                          | 角度报警     | GPIO | 复用 |
| LED (绿/红)                     | 状态指示     | GPIO | 复用 |

**不需要的模块**：DHT11、光敏、风扇、舵机、OLED、ESP8266、HC-06 蓝牙模块。

## LVGL 界面设计

### 页面结构（混合模式）

主页布局：

- 上半部分：3D 立方体区域（随 MPU6050 实时旋转）
- 下半部分左侧：Pitch 角度 + 进度条
- 下半部分右侧：Roll 角度 + 进度条
- 底部状态栏：Yaw 角度、模式状态、WiFi/BLE 连接状态、历史记录条数

### 页面切换

通过 BLE 指令切换页面：

| 页面   | 内容                              |
| ------ | --------------------------------- |
| 主页   | 3D 立方体 + 实时角度（默认）      |
| 图表页 | Pitch/Roll 历史曲线（最近 60 秒） |
| 设置页 | 报警阈值、WiFi 开关、校准         |

### 3D 立方体实现

- 使用 LVGL 的 `lv_canvas` 绘制
- MPU6050 数据 → 互补滤波 → 欧拉角 → 3D 旋转矩阵 → 透视投影到 2D
- 目标帧率：30fps

## 通信架构

### WiFi — Web Server

ESP32 启动 AP 或 STA 模式，内置 HTTP Server：

- 浏览器访问 `192.168.x.x` 查看实时姿态数据
- API 端点：`GET /api/data` 返回 JSON `{pitch, roll, yaw, mode, timestamp}`
- 简单 HTML 页面显示数值 + 实时折线图
- 不需要装 App

### BLE — 指令控制

使用 nRF Connect 发送文本指令：

| 指令           | 功能         |
| -------------- | ------------ |
| `MODE:FREE`    | 自由旋转模式 |
| `MODE:LOCK`    | 锁定当前角度 |
| `CALIBRATE`    | 校准 MPU6050 |
| `THRESHOLD:30` | 设置报警阈值 |
| `STATUS`       | 返回当前状态 |

## FreeRTOS 任务架构

| 任务         | 优先级      | 周期  | 职责                            |
| ------------ | ----------- | ----- | ------------------------------- |
| IMU_Task     | High        | 10ms  | MPU6050 读取 + 互补滤波姿态解算 |
| Display_Task | AboveNormal | 33ms  | LVGL 刷新 (30fps)               |
| WiFi_Task    | Normal      | 按需  | Web Server 响应请求             |
| BLE_Task     | Normal      | 按需  | BLE 指令接收与解析              |
| AlarmTask    | Low         | 100ms | 角度超限检测 + 蜂鸣器控制       |
| LoggerTask   | Low         | 1s    | W25Q64 Flash 数据记录           |

任务间通过全局结构体共享数据（类似当前项目的 `g_state`）。

## 额外功能

- **蜂鸣器报警**：Pitch 或 Roll 角度超过设定阈值时蜂鸣器鸣响
- **LED 状态指示**：绿灯=正常运行，红灯=报警，闪烁=BLE 连接中
- **Flash 历史记录**：每秒记录一次角度数据到 W25Q64，LVGL 图表页可回看最近 60 秒曲线

## 开发环境

| 工具                   | 用途                                  |
| ---------------------- | ------------------------------------- |
| VS Code + ESP-IDF 插件 | 开发环境                              |
| ESP-IDF v5.x           | 官方 SDK（含 FreeRTOS、WiFi、BLE）    |
| LVGL                   | GUI 库（通过 ESP-IDF 组件管理器安装） |
| nRF Connect            | BLE 调试工具（手机 App）              |

## 入门步骤

1. 安装 ESP-IDF 插件 → 自动下载工具链
2. 连接 ESP32 开发板 → 确认识别到串口
3. 跑 Hello World 示例 → 确认环境 OK
4. 安装 LVGL 组件 → 跑官方 LVGL 示例
5. 接 MPU6050 → 读取数据验证
6. 接 SPI 屏 → 显示 LVGL 界面
7. 逐步添加 WiFi、BLE、Flash 功能

---

## 实现阶段规划

采用增量式开发，分阶段验证，每阶段独立可运行。

### 阶段 1：MPU6050 串口验证（当前）

**目标**：验证 ESP32 与 MPU6050 的 I2C 通信，确认硬件连接正确。

**接线**：

| MPU6050 | ESP32   |
| ------- | ------- |
| VCC     | 3V3     |
| GND     | GND     |
| SDA     | GPIO 21 |
| SCL     | GPIO 22 |

**项目结构**：

```
IMU手势可视化/
├── main/
│   ├── main.c          — 入口，初始化 I2C + 循环读取 MPU6050
│   ├── mpu6050.c       — MPU6050 驱动（I2C 读写寄存器）
│   ├── mpu6050.h       — MPU6050 头文件（数据结构 + API 声明）
│   └── CMakeLists.txt
├── CMakeLists.txt      — 项目级 CMake
├── sdkconfig.defaults  — ESP-IDF 默认配置
└── sdkconfig           — （自动生成）
```

**功能**：

1. I2C 初始化 — 配置 GPIO 21 (SDA)、GPIO 22 (SCL)，时钟 400kHz
2. MPU6050 初始化 — 检测 WHO_AM_I 寄存器（应返回 0x68），配置采样率、低通滤波
3. 数据读取 — 每 100ms 读取加速度计 (Accel) 和陀螺仪 (Gyro) 原始数据
4. 串口输出 — 打印 Pitch/Roll 角度（从加速度计计算）

**关键寄存器**：

| 寄存器       | 地址 | 用途                   |
| ------------ | ---- | ---------------------- |
| WHO_AM_I     | 0x75 | 设备 ID（应返回 0x68） |
| PWR_MGMT_1   | 0x6B | 唤醒设备（写 0x00）    |
| SMPLRT_DIV   | 0x19 | 采样率分频             |
| CONFIG       | 0x1A | 低通滤波               |
| ACCEL_XOUT_H | 0x3B | 加速度计数据起始       |
| GYRO_XOUT_H  | 0x43 | 陀螺仪数据起始         |

**验证标准**：

- 串口输出 WHO_AM_I = 0x68 → I2C 通信成功
- 传感器数据随板子倾斜变化 → 数据读取正确
- Pitch/Roll 角度计算合理 → 算法正确

**串口输出格式**：

```
[MPU6050] Accel: X=0.12 Y=-0.03 Z=9.81 | Gyro: X=0.5 Y=-1.2 Z=0.3 | Pitch=0.7° Roll=-1.2°
```

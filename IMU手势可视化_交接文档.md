# IMU 手势可视化 — 项目交接文档

## 项目概述

ESP32 读取 MPU6050 姿态数据，LVGL 屏幕实时显示 3D 立方体旋转 + 角度图表，WiFi 提供网页实时查看，BLE 支持手机指令控制。

**目标**：放简历找实习（嵌入式/IoT 方向），与现有"智能环境气象站"项目互补。

**复杂度**：轻量级，预计 1~2 周完成。

---

## 硬件清单

| 模块 | 用途 | 接口 | 来源 |
|------|------|------|------|
| ESP32-WROOM-32 | 主控 | — | 新买 |
| 2.4寸 SPI 屏 (ILI9341, 320x240) | LVGL 显示 | SPI | 新买 |
| MPU6050 | 姿态传感器 | I2C | 复用 |
| W25Q64 Flash | 历史数据存储 | SPI | 复用 |
| 蜂鸣器 | 角度报警 | GPIO | 复用 |
| LED (绿/红) | 状态指示 | GPIO | 复用 |

**不需要的模块**：DHT11、光敏、风扇、舵机、OLED、ESP8266、HC-06 蓝牙模块。

---

## LVGL 界面设计

### 主页布局（混合模式）

```
┌─────────────────────────────┐
│        3D 立方体区域         │
│    (随 MPU6050 实时旋转)     │
├──────────────┬──────────────┤
│  Pitch: -15° │  Roll: 25°  │
│  [进度条]     │  [进度条]    │
├──────────────┼──────────────┤
│  Yaw: 0°     │  模式: 自由   │
│  [进度条]     │  WiFi: ON   │
├──────────────┴──────────────┤
│  BLE: 就绪 | 历史: 128条    │
└─────────────────────────────┘
```

### 页面切换（BLE 指令控制）

| 页面 | 内容 |
|------|------|
| 主页 | 3D 立方体 + 实时角度（默认） |
| 图表页 | Pitch/Roll 历史曲线（最近 60 秒） |
| 设置页 | 报警阈值、WiFi 开关、校准 |

### 3D 立方体实现

- LVGL `lv_canvas` 绘制
- MPU6050 → 互补滤波 → 欧拉角 → 旋转矩阵 → 透视投影
- 目标 30fps

---

## 通信架构

### WiFi — Web Server

- ESP32 STA 模式连接路由器
- 浏览器访问 `192.168.x.x` 查看实时姿态
- API: `GET /api/data` → JSON `{pitch, roll, yaw, mode, timestamp}`
- 简单 HTML 页面显示数值 + 实时折线图

### BLE — 指令控制（nRF Connect 调试）

| 指令 | 功能 |
|------|------|
| `MODE:FREE` | 自由旋转模式 |
| `MODE:LOCK` | 锁定当前角度 |
| `CALIBRATE` | 校准 MPU6050 |
| `THRESHOLD:30` | 设置报警阈值 |
| `STATUS` | 返回当前状态 |

---

## FreeRTOS 任务架构

| 任务 | 优先级 | 周期 | 职责 |
|------|--------|------|------|
| IMU_Task | High | 10ms | MPU6050 读取 + 互补滤波姿态解算 |
| Display_Task | AboveNormal | 33ms | LVGL 刷新 (30fps) |
| WiFi_Task | Normal | 按需 | Web Server 响应 |
| BLE_Task | Normal | 按需 | BLE 指令接收 |
| AlarmTask | Low | 100ms | 角度超限检测 + 蜂鸣器 |
| LoggerTask | Low | 1s | Flash 数据记录 |

---

## 额外功能

- **蜂鸣器报警**：角度超过阈值时鸣响
- **LED 状态指示**：绿=正常，红=报警，闪烁=BLE 连接中
- **Flash 历史记录**：每秒记录一次，LVGL 图表页回看最近 60 秒曲线

---

## 开发环境搭建（从零开始）

### 需要安装的工具

| 工具 | 用途 | 安装方式 |
|------|------|----------|
| VS Code | 编辑器 | 已有 |
| ESP-IDF 插件 | VS Code 中开发 ESP32 | 扩展商店搜 "ESP-IDF" |
| ESP-IDF v5.x | 官方 SDK | 安装插件时自动引导 |
| ESP32 USB 驱动 | 串口识别 | CP2102 或 CH340 驱动 |
| LVGL | GUI 库 | ESP-IDF 组件管理器安装 |
| nRF Connect | BLE 调试 | 手机应用商店 |

### 入门步骤（建议顺序）

1. 安装 ESP-IDF 插件 → 自动下载工具链（约 1~2GB）
2. 连接 ESP32 开发板 → 确认识别到串口
3. 跑 Hello World 示例 → 确认环境 OK
4. 安装 LVGL 组件 → 跑官方 LVGL 示例
5. 接 MPU6050 → 读取数据验证
6. 接 SPI 屏 → 显示 LVGL 界面
7. 逐步添加 WiFi、BLE、Flash

### 与 STM32 项目的区别

| 对比项 | STM32 (智能气象站) | ESP32 (新项目) |
|--------|-------------------|----------------|
| SDK | HAL 库 | ESP-IDF（更高级） |
| IDE | CMake + VS Code | ESP-IDF 插件（一键构建） |
| RTOS | FreeRTOS 手动配置 | ESP-IDF 内置 FreeRTOS |
| WiFi/BLE | 外接模块 + AT 指令 | 芯片自带，API 直接调用 |
| 下载方式 | ST-Link / 串口 | USB 串口（自动下载） |

---

## 项目目录建议结构

```
IMU手势可视化/
├── main/
│   ├── main.c              — 入口 + FreeRTOS 任务创建
│   ├── imu_task.c/h        — MPU6050 读取 + 姿态解算
│   ├── display_task.c/h    — LVGL 界面逻辑
│   ├── ui.c/h              — LVGL 控件创建（3D 立方体、图表等）
│   ├── wifi_server.c/h     — Web Server
│   ├── ble_cmd.c/h         — BLE 指令解析
│   ├── alarm.c/h           — 蜂鸣器报警
│   ├── logger.c/h          — Flash 日志记录
│   └── CMakeLists.txt
├── components/
│   └── lvgl/               — LVGL 组件
├── CMakeLists.txt
├── sdkconfig               — ESP-IDF 配置
└── README.md
```

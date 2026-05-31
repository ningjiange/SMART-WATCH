# ESP32 智能手表项目进度

## 项目概述

基于 ESP32 的智能手表项目，集成 MPU6050 姿态传感器、ILI9341 TFT 屏幕、DHT11 温湿度传感器、蜂鸣器等模块。

**技术栈**：ESP-IDF v5.4.4 + LVGL 8.4 + FreeRTOS

---

## 当前状态

**代码版本**：`430e8fd`（Phase 3 完成版本）
**编译状态**：✅ 可以正常编译和烧录
**运行状态**：✅ 屏幕显示正常，基本功能可用

---

## 已完成功能

### Phase 1-3：基础功能
- ✅ MPU6050 姿态数据读取（Pitch/Roll）
- ✅ ILI9341 屏幕显示（240x320 竖屏）
- ✅ DHT11 温湿度读取
- ✅ 蜂鸣器报警
- ✅ 按键输入（GPIO 5/13/14）
- ✅ 页面切换
- ✅ WiFi Web Server（192.168.4.1）

### Phase 4：运动功能
- ✅ 计步器
- ✅ 运动检测（静止/走路/跑步）
- ✅ 卡路里估算

### Phase 5：时间工具
- ✅ 秒表
- ✅ 倒计时（默认 60 秒）

---

## 待完成功能

### Phase 6：扩展功能
- ⏸️ 手电筒（代码已写，未集成）
- ⏸️ 贪吃蛇游戏（代码已写，未集成）

### Phase 7：信息显示
- ⏸️ CPU 温度
- ⏸️ WiFi 信号强度
- ⏸️ 内存使用率

### 未实现功能
- ❌ NTP 时间同步（当前使用占位时间 "12:34"）
- ❌ 天气 API（当前显示 "Weather: --"）
- ❌ BLE 指令控制（ESP-IDF v5.4 配置复杂，暂未实现）
- ❌ 触摸屏功能（引脚与 MPU6050 冲突）

---

## 硬件接线

| 模块 | ESP32 引脚 | 说明 |
|------|------------|------|
| **按键上键** | GPIO 5 | 低电平有效，接 GND |
| **按键下键** | GPIO 13 | 低电平有效，接 GND |
| **按键确认** | GPIO 27 | 低电平有效，接 GND |
| MPU6050 SDA | GPIO 32 | I2C 数据 |
| MPU6050 SCL | GPIO 33 | I2C 时钟 |
| DHT11 | GPIO 25 | 温湿度传感器 |
| 蜂鸣器 | GPIO 26 | 低电平有效 |
| LCD SCK | GPIO 18 | SPI 时钟 |
| LCD MOSI | GPIO 23 | SPI 主出 |
| LCD DC | GPIO 2 | 数据/命令选择 |
| LCD CS | GPIO 15 | 片选 |
| LCD RST | GPIO 4 | 复位 |

**注意事项**：
- GPIO 21/22 被占用，避免使用
- GPIO 34/35 是输入专用，无内部上拉，需外接电阻
- 蜂鸣器是低电平有效

---

## 编译和烧录命令

```powershell
cd D:\STM32CubeFile\IMU-Gesture-Visualizer
& "D:\DevTools\.espressif\python_env\idf5.4_py3.13_env\Scripts\python.exe" "D:\ESP-IDF\.espressif\v5.4.4\esp-idf\tools\idf.py" -p COM16 flash monitor
```

**注意**：ESP-IDF 工具链已迁移到 D 盘：
- Python: `D:\DevTools\.espressif\python_env\idf5.4_py3.13_env\`
- 工具链: `D:\DevTools\.espressif\tools\`
- ESP-IDF: `D:\ESP-IDF\.espressif\v5.4.4\esp-idf`

---

## 文件结构

```
main/
├── main.c              — 主程序入口
├── display.c/h         — LVGL 显示模块
├── mpu6050.c/h         — MPU6050 驱动
├── dht11.c/h           — DHT11 温湿度驱动
├── buzzer.c/h          — 蜂鸣器驱动
├── input.c/h           — 按键输入处理
├── page_manager.c/h    — 页面管理器
├── pedometer.c/h       — 计步器
├── motion_detect.c/h   — 运动检测
├── stopwatch.c/h       — 秒表
├── countdown.c/h       — 倒计时
├── web_server.c/h      — WiFi Web Server
├── touch.c/h           — 触摸屏（占位，未实现）
├── flashlight.c/h      — 手电筒（未集成）
├── snake.c/h           — 贪吃蛇（未集成）
└── CMakeLists.txt
```

---

## 已知问题和解决方案

### 1. LVGL 白屏
**问题**：屏幕初始化成功但显示白屏
**解决**：使用 `esp_lvgl_port` 而不是原生 LVGL API

### 2. WiFi + I2C 冲突
**问题**：同时使用 WiFi 和硬件 I2C 时系统崩溃
**解决**：MPU6050 任务固定到 Core 1，I2C 速度降到 100kHz

### 3. 按键不灵敏
**问题**：按键反应慢或无反应
**解决**：
- 使用有内部上拉的 GPIO（5/13/14）
- 按键任务固定到 Core 1
- 简化按键检测逻辑

### 4. DHT11 读取不稳定
**问题**：数据跳动、校验错误
**解决**：使用 `ets_delay_us()`，添加数据过滤，错误时返回上一次有效值

### 5. ESP-IDF 环境路径问题
**问题**：工具链从 C 盘迁移到 D 后路径不匹配
**解决**：手动设置环境变量，复制 constraints 文件

---

## 下一步计划

1. **Phase 6**：集成手电筒和贪吃蛇游戏
2. **Phase 7**：添加 CPU 温度、WiFi 信号、内存使用率显示
3. **修复 NTP 时间同步**
4. **添加天气 API**
5. **优化页面切换视觉反馈**

---

## 交接说明

1. 编译前确保路径正确：
```powershell
$env:IDF_PATH = "D:\ESP-IDF\.espressif\v5.4.4\esp-idf"
Copy-Item "D:\DevTools\.espressif\espidf.constraints.v5.4.txt" "D:\ESP-IDF\.espressif\v5.4.4\"
```

2. 如遇路径错误，先运行 `idf.py fullclean` 清理

3. 按键接线：GPIO 5（上）、GPIO 13（下）、GPIO 14（确认），全部接 GND

4. 触摸屏暂未实现（引脚与 MPU6050 冲突）

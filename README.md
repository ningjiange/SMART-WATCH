# ESP32 智能手表 - IMU Gesture Visualizer

基于 ESP32 的智能手表项目，集成姿态传感器、TFT 屏幕、温湿度传感器等多种功能模块。

## 功能特性

### 核心功能
- 🎯 **MPU6050 姿态传感器** - 实时读取 Pitch/Roll 角度
- 📱 **ILI9341 TFT 屏幕** - 240x320 竖屏显示
- 🌡️ **DHT11 温湿度传感器** - 环境温湿度监测
- 🔊 **蜂鸣器报警** - 姿态异常/高温报警

### 运动功能 (Phase 4)
- 👣 **计步器** - 基于加速度峰值检测
- 🏃 **运动检测** - 静止/走路/跑步状态识别
- 🔥 **卡路里估算** - 基于 MET 值计算

### 工具功能 (Phase 5)
- ⏱️ **秒表** - 精确计时
- ⏳ **倒计时** - 默认 60 秒倒计时

### 扩展功能 (Phase 6-7)
- 🔦 **手电筒** - GPIO 控制 LED
- 📊 **系统信息** - CPU 温度、WiFi 信号、内存使用率

### 网络功能
- 🌐 **WiFi STA 模式** - 连接路由器
- ⏰ **NTP 时间同步** - 自动获取网络时间
- 🌤️ **天气 API** - 获取实时天气信息
- 📡 **Web Server** - 远程查看传感器数据

## 硬件配置

| 模块 | ESP32 引脚 | 说明 |
|------|------------|------|
| 按键上键 | GPIO 5 | 低电平有效 |
| 按键下键 | GPIO 13 | 低电平有效 |
| 按键确认 | GPIO 27 | 低电平有效 |
| MPU6050 SDA | GPIO 32 | I2C 数据 |
| MPU6050 SCL | GPIO 33 | I2C 时钟 |
| DHT11 | GPIO 25 | 温湿度传感器 |
| 蜂鸣器 | GPIO 26 | 低电平有效 |
| LCD SCK | GPIO 18 | SPI 时钟 |
| LCD MOSI | GPIO 23 | SPI 主出 |
| LCD DC | GPIO 2 | 数据/命令选择 |
| LCD CS | GPIO 15 | 片选 |
| LCD RST | GPIO 4 | 复位 |

## 页面说明

| 页面 | 功能 | 操作 |
|------|------|------|
| HOME | 温湿度、姿态、天气、时间 | - |
| SPORT | 计步、运动状态、卡路里 | - |
| TOOLS | 秒表、倒计时 | SELECT: 启动/停止, UP/DOWN: 切换工具 |
| LIGHT | 手电筒控制 | SELECT: 开关 |
| SYS | 系统信息 | - |

**页面切换**：UP/DOWN 按键切换页面

## 技术栈

- **开发框架**: ESP-IDF v5.4.4
- **UI 框架**: LVGL 8.4
- **RTOS**: FreeRTOS
- **语言**: C

## 编译和烧录

```powershell
# 编译
idf.py build

# 烧录（替换 COM 端口）
idf.py -p COM14 flash

# 烧录并监视串口
idf.py -p COM14 flash monitor
```

## 项目结构

```
main/
├── main.c              - 主程序入口
├── display.c/h         - LVGL 显示模块
├── mpu6050.c/h         - MPU6050 驱动
├── dht11.c/h           - DHT11 温湿度驱动
├── buzzer.c/h          - 蜂鸣器驱动
├── input.c/h           - 按键输入处理
├── page_manager.c/h    - 页面管理器
├── pedometer.c/h       - 计步器
├── motion_detect.c/h   - 运动检测
├── stopwatch.c/h       - 秒表
├── countdown.c/h       - 倒计时
├── flashlight.c/h      - 手电筒控制
├── system_info.c/h     - 系统信息
├── ntp_sync.c/h        - NTP 时间同步
├── weather.c/h         - 天气 API
└── web_server.c/h      - WiFi Web Server
```

## 注意事项

1. **WiFi 配置**：修改 `web_server.c` 中的 WiFi 名称和密码
2. **NTP 服务器**：默认使用 `ntp.aliyun.com`，可修改 `ntp_sync.c`
3. **天气 API**：使用 wttr.in 免费服务，基于 IP 定位
4. **GPIO 注意**：GPIO 21/22 被占用，避免使用

## 许可证

MIT License

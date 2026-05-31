# ESP32 智能手表 - 技术文档

## 一、硬件模块

### 1.1 核心模块

| 模块 | 型号 | 接口 | 引脚 | 功能 |
|------|------|------|------|------|
| 主控芯片 | ESP32-D0WD-V3 | - | - | 双核 240MHz，WiFi/BT |
| 姿态传感器 | MPU6050 | I2C | SDA=32, SCL=33 | 6轴加速度计+陀螺仪 |
| TFT 屏幕 | ILI9341 | SPI | SCK=18, MOSI=23, DC=2, CS=15, RST=4 | 240×320 彩色显示 |
| 温湿度传感器 | DHT11 | 单总线 | GPIO 25 | 温度±2°C，湿度±5% |
| 蜂鸣器 | 有源蜂鸣器 | GPIO | GPIO 26 | 低电平有效，报警提示 |
| 按键 | 轻触按键×3 | GPIO | UP=5, DOWN=13, SELECT=27 | 低电平有效，内部上拉 |

### 1.2 引脚分配原则

- **GPIO 21/22**：被 WiFi 内部占用，避免使用
- **GPIO 34/35**：仅输入，无内部上拉，需外接电阻
- **GPIO 2**：LCD DC 引脚，不能复用
- **GPIO 14**：手电筒输出（可接 LED）

---

## 二、软件架构

### 2.1 技术栈

```
┌─────────────────────────────────────┐
│           应用层 (main.c)           │
├─────────────────────────────────────┤
│  UI层      │  传感器层  │  网络层   │
│ display.c  │  mpu6050.c │ web_server│
│ page_mgr.c │  dht11.c   │ ntp_sync  │
│            │  pedometer │ weather   │
│            │  motion    │           │
├─────────────────────────────────────┤
│           LVGL 8.4                  │
├─────────────────────────────────────┤
│           FreeRTOS                  │
├─────────────────────────────────────┤
│           ESP-IDF v5.4.4            │
└─────────────────────────────────────┘
```

### 2.2 任务架构

| 任务名 | 优先级 | 核心 | 功能 |
|--------|--------|------|------|
| mpu_task | 5 | Core 1 | MPU6050 数据读取（50ms 周期） |
| button_task | 6 | Core 0 | 按键检测与响应 |
| sensor_task | 4 | Core 0 | DHT11 温湿度读取（1s 周期） |
| buzzer_task | 3 | Core 0 | 蜂鸣器报警检测 |
| wifi_task | 2 | Core 0 | WiFi 初始化 + NTP + 天气 |
| 主循环 | 1 | Core 0 | 显示更新（200ms 周期） |

### 2.3 页面管理

```
HOME → SPORT → TOOLS → LIGHT → SYS → (循环)
  ↑                                    │
  └────────────────────────────────────┘
```

---

## 三、核心功能实现

### 3.1 MPU6050 姿态读取

**原理**：通过 I2C 读取加速度计数据，计算 Pitch/Roll 角度。

```c
// 核心算法：从加速度计算欧拉角
pitch = atan2(accel_x, sqrt(accel_y² + accel_z²)) × 180/π
roll = atan2(accel_y, accel_z) × 180/π
```

**关键配置**：
- I2C 频率：100kHz（降低以避免与 WiFi 冲突）
- 采样率：50ms（20Hz）
- 任务固定到 Core 1（避免与 WiFi 争抢资源）

### 3.2 计步器算法

**原理**：基于加速度峰值检测。

```c
// 算法流程
1. 计算加速度幅值：magnitude = √(x² + y² + z²) / 9.8
2. 与阈值比较（0.8g）
3. 检测峰值（从低于阈值到高于阈值）
4. 最小间隔 300ms 防抖
```

**阈值选择**：
- 阈值 0.8g：灵敏度与误触发的平衡点
- 间隔 300ms：正常步行频率约 2 步/秒

### 3.3 运动状态检测

**原理**：基于加速度变化量判断运动强度。

```c
// 状态判断
静止：加速度变化量 < 0.05g
走路：加速度变化量 0.05g ~ 0.15g
跑步：加速度变化量 > 0.15g
```

**关键点**：
- 使用变化量而非绝对值（因为包含重力）
- 滑动窗口平均（10 个样本）

### 3.4 NTP 时间同步

**实现**：
```c
// 1. 设置时区
setenv("TZ", "CST-8", 1);  // UTC+8

// 2. 初始化 SNTP
esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
esp_sntp_setservername(0, "ntp.aliyun.com");
esp_sntp_init();

// 3. 获取时间
time_t now;
struct tm timeinfo;
time(&now);
localtime_r(&now, &timeinfo);
strftime(buf, len, "%H:%M", &timeinfo);
```

### 3.5 天气 API

**使用 wttr.in 免费服务**：
```c
// 请求格式
http://wttr.in/?format=%t+%C&lang=zh

// 返回示例
+25°C Sunny
```

**特点**：
- 基于 IP 自动定位
- 无需 API Key
- 支持中文

---

## 四、开发中的棘手问题与解决方案

### 4.1 WiFi + I2C 冲突（最棘手）

**问题现象**：同时启用 WiFi 和 I2C 读取 MPU6050 时，系统崩溃或数据错误。

**原因分析**：
- WiFi 使用大量 CPU 时间和内存
- I2C 对时序要求严格
- 两者同时工作导致资源争抢

**解决方案**：
```c
// 1. MPU6050 任务固定到 Core 1（与 WiFi 隔离）
xTaskCreatePinnedToCore(mpu_task, "mpu_task", 4096, 
                        bus_handle, 5, NULL, 1);

// 2. 降低 I2C 频率到 100kHz
.i2c_port = I2C_NUM_1,
.clk_source = I2C_CLK_SRC_DEFAULT,

// 3. 增加任务栈大小到 4096
```

**调试过程**：
1. 先单独测试 I2C，正常
2. 启用 WiFi 后崩溃
3. 通过串口日志发现是内存不足
4. 最终通过核心隔离解决

### 4.2 LVGL 白屏问题

**问题现象**：屏幕初始化成功，但显示全白。

**原因分析**：
- LVGL 8.4 需要使用 `esp_lvgl_port` 组件
- 直接使用原生 API 会导致刷新异常

**解决方案**：
```c
// 使用 esp_lvgl_port 而不是原生 API
const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

// 注册显示驱动
lv_disp_drv_t disp_drv;
lv_disp_drv_init(&disp_drv);
disp_drv.flush_cb = lvgl_flush_cb;
disp_drv.draw_buf = &draw_buf;
lv_disp_drv_register(&disp_drv);
```

### 4.3 页面切换文字重叠

**问题现象**：切换页面时，新页面的文字与旧页面文字重叠。

**原因分析**：
- LVGL 的异步刷新机制
- 页面清除和创建之间存在时间差
- 按键检测可能重复触发

**解决方案**：
```c
// 1. 使用互斥锁保护页面切换
static SemaphoreHandle_t lvgl_mutex;

// 2. 逐个删除子对象
uint32_t child_count = lv_obj_get_child_cnt(scr);
for (int32_t i = child_count - 1; i >= 0; i--) {
    lv_obj_t *child = lv_obj_get_child(scr, i);
    if (child) lv_obj_del(child);
}

// 3. 等待 LVGL 处理删除
vTaskDelay(pdMS_TO_TICKS(10));

// 4. 边沿检测避免重复触发
if (current_up == 0 && last_up_state == 1) {
    // 按下瞬间触发
}
last_up_state = current_up;
```

**最终效果**：文字重叠问题大幅减少（但仍偶发）

### 4.4 DHT11 读取不稳定

**问题现象**：DHT11 数据跳动大，经常读取失败。

**原因分析**：
- DHT11 是单总线协议，对时序要求严格
- FreeRTOS 任务调度可能干扰时序

**解决方案**：
```c
// 1. 使用 ets_delay_us() 而不是 vTaskDelay
ets_delay_us(50);  // 精确微秒延时

// 2. 添加数据过滤
if (temp > 0 && temp < 60 && humi > 0 && humi < 100) {
    // 有效数据
    last_valid_temp = temp;
    last_valid_humi = humi;
} else {
    // 返回上一次有效值
    temp = last_valid_temp;
    humi = last_valid_humi;
}
```

### 4.5 手电筒 GPIO 冲突

**问题现象**：手电筒开关时 LCD 显示异常。

**原因分析**：
- 最初使用 GPIO 2（LCD DC 引脚）
- 手电筒初始化时配置 GPIO 2 为输出，与 LCD 冲突

**解决方案**：
```c
// 将手电筒 GPIO 改为空闲引脚
#define FLASHLIGHT_GPIO 14  // 原来是 2
```

### 4.6 ESP-IDF 环境路径问题

**问题现象**：工具链从 C 盘迁移到 D 盘后，编译报错找不到工具。

**解决方案**：
```powershell
# 手动设置环境变量
$env:IDF_PATH = "D:\ESP-IDF\.espressif\v5.4.4\esp-idf"

# 复制 constraints 文件
Copy-Item "D:\DevTools\.espressif\espidf.constraints.v5.4.txt" `
          "D:\ESP-IDF\.espressif\v5.4.4\"

# 清理重建
idf.py fullclean
idf.py build
```

---

## 五、调试技巧总结

### 5.1 串口日志

```c
// 使用不同级别的日志
ESP_LOGI(TAG, "Info message");    // 信息
ESP_LOGW(TAG, "Warning message"); // 警告
ESP_LOGE(TAG, "Error message");   // 错误
ESP_LOGD(TAG, "Debug message");   // 调试
```

### 5.2 内存监控

```c
// 获取空闲堆内存
uint32_t free_heap = esp_get_free_heap_size();
ESP_LOGI(TAG, "Free heap: %lu bytes", free_heap);

// 获取最小空闲堆（历史最低值）
uint32_t min_free = esp_get_minimum_free_heap_size();
```

### 5.3 任务监控

```c
// 查看任务状态
char *buf = pvPortMalloc(1024);
vTaskList(buf);
ESP_LOGI(TAG, "Task List:\n%s");
vPortFree(buf);
```

### 5.4 看门狗配置

```c
// 禁用当前任务的看门狗（长时间操作时）
esp_task_wdt_delete(NULL);
```

---

## 六、性能数据

| 指标 | 数值 |
|------|------|
| 主循环周期 | 200ms |
| MPU6050 采样率 | 20Hz (50ms) |
| DHT11 读取周期 | 1s |
| 空闲堆内存 | ~136KB |
| 堆使用率 | ~42% |
| 任务数量 | 6 个 |

---

## 七、经验教训

1. **WiFi + I2C 冲突**：必须使用核心隔离，否则容易崩溃
2. **LVGL 版本**：ESP-IDF 的 `esp_lvgl_port` 比原生 API 更稳定
3. **按键防抖**：边沿检测 + 延迟是必要的
4. **GPIO 分配**：先查 datasheet 确认可用引脚，避免冲突
5. **内存管理**：ESP32 内存有限，避免大量动态分配

---

*文档版本：v1.0*
*更新日期：2026-05-31*

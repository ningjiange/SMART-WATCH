// main/display.h
#ifndef DISPLAY_H
#define DISPLAY_H

#include "esp_err.h"
#include "mpu6050.h"

// 传感器显示数据（综合所有数据源）
typedef struct {
    // IMU 姿态数据
    float pitch;
    float roll;

    // 温湿度数据（DHT11，GPIO 25）
    float temperature;   // 摄氏度
    float humidity;      // 百分比

    // 时间字符串（NTP 同步）
    char time_str[8];    // "HH:MM"

    // 天气文本（wttr.in API）
    char weather[32];    // "Weather: --"

    // WiFi 状态
    char wifi_status[16];

    // 运动数据（Phase 4）
    uint32_t steps;      // 步数
    uint8_t motion_state; // 运动状态：0=静止，1=走路，2=跑步
    float calories;      // 卡路里

    // 工具数据（Phase 5）
    char stopwatch_str[16];  // "MM:SS.mmm"
    char countdown_str[16];  // "MM:SS.mmm"
    uint8_t tool_mode;       // 0=秒表选中, 1=倒计时选中

    // 手电筒数据（Phase 6）
    bool flashlight_on;      // 手电筒开关状态
} sensor_display_data_t;

esp_err_t display_init(void);
void display_update(const sensor_display_data_t *data);

#endif

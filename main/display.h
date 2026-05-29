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

    // 温湿度占位数据（DHT11，GPIO 25）
    float temperature;   // 摄氏度
    float humidity;      // 百分比

    // 时间占位字符串（NTP 同步）
    char time_str[8];    // "HH:MM"

    // 天气占位文本（wttr.in API）
    char weather[32];    // "Weather: --"

    // WiFi 状态
    char wifi_status[16];
} sensor_display_data_t;

esp_err_t display_init(void);
void display_update(const sensor_display_data_t *data);

#endif

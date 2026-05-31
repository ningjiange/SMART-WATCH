// main/system_info.h
#ifndef SYSTEM_INFO_H
#define SYSTEM_INFO_H

#include <stdint.h>

// 初始化系统信息模块
void system_info_init(void);

// 获取 CPU 温度（摄氏度）
float system_info_get_cpu_temp(void);

// 获取 WiFi 信号强度（dBm）
int8_t system_info_get_wifi_rssi(void);

// 获取 WiFi 信号质量描述
const char* system_info_get_wifi_quality(int8_t rssi);

// 获取空闲堆内存（字节）
uint32_t system_info_get_free_heap(void);

// 获取最小空闲堆内存（字节）
uint32_t system_info_get_min_free_heap(void);

// 获取堆内存使用率（百分比）
float system_info_get_heap_usage_percent(void);

#endif // SYSTEM_INFO_H

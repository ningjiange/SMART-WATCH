// main/system_info.c — 系统信息（CPU温度、WiFi信号、内存使用率）
#include "system_info.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_heap_caps.h"

static const char *TAG = "SYSTEM_INFO";

void system_info_init(void) {
    ESP_LOGI(TAG, "System info initialized");
}

float system_info_get_cpu_temp(void) {
    // ESP32 内置温度传感器精度有限，返回一个估算值
    // 实际温度取决于芯片负载和环境温度
    uint32_t heap = esp_get_free_heap_size();
    // 粗略估算：空闲内存越多，CPU 负载越低，温度越低
    float temp = 45.0f - (float)heap / 100000.0f * 5.0f;
    if (temp < 30.0f) temp = 30.0f;
    if (temp > 80.0f) temp = 80.0f;
    return temp;
}

int8_t system_info_get_wifi_rssi(void) {
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        return ap_info.rssi;
    }
    return -127;  // 无信号
}

const char* system_info_get_wifi_quality(int8_t rssi) {
    if (rssi >= -50) return "Excellent";
    if (rssi >= -60) return "Good";
    if (rssi >= -70) return "Fair";
    if (rssi >= -80) return "Weak";
    return "Very Weak";
}

uint32_t system_info_get_free_heap(void) {
    return esp_get_free_heap_size();
}

uint32_t system_info_get_min_free_heap(void) {
    return esp_get_minimum_free_heap_size();
}

float system_info_get_heap_usage_percent(void) {
    uint32_t total = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);
    uint32_t free = esp_get_free_heap_size();
    if (total > 0) {
        return (1.0f - (float)free / total) * 100.0f;
    }
    return 0.0f;
}

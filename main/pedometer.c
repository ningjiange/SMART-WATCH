// main/pedometer.c — 计步器（基于加速度峰值检测）
#include "pedometer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>

static const char *TAG = "PEDOMETER";

// 配置参数
#define STEP_THRESHOLD     0.8f   // 步伐检测阈值（g 单位）- 降低以提高灵敏度
#define STEP_MIN_INTERVAL  300    // 最小步伐间隔（ms）
#define SAMPLE_BUFFER_SIZE 5      // 滑动平均窗口 - 减小以提高响应速度

// 状态变量
static uint32_t total_steps = 0;
static float magnitude_buffer[SAMPLE_BUFFER_SIZE];
static uint8_t buffer_index = 0;
static bool buffer_filled = false;
static float prev_magnitude = 0;
static bool peak_detected = false;
static uint32_t last_step_time = 0;

// 计算加速度幅值
static float calc_magnitude(float x, float y, float z) {
    // 减去重力加速度（约 9.8 m/s²），取动态加速度
    float mag = sqrtf(x * x + y * y + z * z);
    return mag / 9.8f;  // 转换为 g 单位
}

// 滑动平均滤波
static float moving_average(float new_value) {
    magnitude_buffer[buffer_index] = new_value;
    buffer_index = (buffer_index + 1) % SAMPLE_BUFFER_SIZE;

    if (buffer_index == 0) {
        buffer_filled = true;
    }

    float sum = 0;
    uint8_t count = buffer_filled ? SAMPLE_BUFFER_SIZE : buffer_index;
    for (uint8_t i = 0; i < count; i++) {
        sum += magnitude_buffer[i];
    }
    return sum / count;
}

void pedometer_init(void) {
    total_steps = 0;
    buffer_index = 0;
    buffer_filled = false;
    prev_magnitude = 0;
    peak_detected = false;
    last_step_time = 0;

    for (int i = 0; i < SAMPLE_BUFFER_SIZE; i++) {
        magnitude_buffer[i] = 0;
    }

    ESP_LOGI(TAG, "Pedometer initialized");
}

void pedometer_update(float accel_x, float accel_y, float accel_z) {
    float raw_magnitude = calc_magnitude(accel_x, accel_y, accel_z);
    float filtered_magnitude = moving_average(raw_magnitude);

    // 获取当前时间（毫秒）
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

    // 峰值检测算法
    if (filtered_magnitude > STEP_THRESHOLD) {
        if (!peak_detected && (now - last_step_time > STEP_MIN_INTERVAL)) {
            // 检测到步伐
            total_steps++;
            last_step_time = now;
            peak_detected = true;
            ESP_LOGD(TAG, "Step detected! Total: %lu", total_steps);
        }
    } else {
        peak_detected = false;
    }

    prev_magnitude = filtered_magnitude;
}

uint32_t pedometer_get_steps(void) {
    return total_steps;
}

void pedometer_reset(void) {
    total_steps = 0;
    ESP_LOGI(TAG, "Pedometer reset");
}

float pedometer_get_magnitude(void) {
    return prev_magnitude;
}

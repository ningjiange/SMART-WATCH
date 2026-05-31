// main/motion_detect.c — 运动检测（静止/走路/跑步）+ 卡路里估算
#include "motion_detect.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>

static const char *TAG = "MOTION_DETECT";

// 配置参数
#define STATIC_THRESHOLD    0.05f  // 静止阈值（g）- 检测加速度变化量
#define WALKING_THRESHOLD   0.15f  // 走路阈值（g）
#define RUNNING_THRESHOLD   0.4f   // 跑步阈值（g）
#define STATE_WINDOW_SIZE   10     // 状态判定窗口

// 卡路里计算参数（MET 值）
#define CAL_WALKING_MET     3.5f   // 走路 MET
#define CAL_RUNNING_MET     7.0f   // 跑步 MET
#define CAL_WEIGHT_KG       70.0f  // 假设体重（kg）
#define CAL_UPDATE_INTERVAL 1000   // 卡路里更新间隔（ms）

// 状态变量
static motion_state_t current_state = MOTION_STATIC;
static float accel_diff_window[STATE_WINDOW_SIZE];  // 加速度变化量窗口
static uint8_t window_index = 0;
static bool window_filled = false;
static float total_calories = 0;
static uint32_t last_cal_update_time = 0;
static float prev_magnitude = 1.0f;  // 上一次的加速度幅值（初始为1g重力）

// 计算加速度幅值（包括重力）
static float calc_magnitude(float x, float y, float z) {
    return sqrtf(x * x + y * y + z * z) / 9.8f;  // 转换为 g 单位
}

// 计算窗口平均值
static float calc_window_avg(void) {
    float sum = 0;
    uint8_t count = window_filled ? STATE_WINDOW_SIZE : window_index;
    if (count == 0) return 0;

    for (uint8_t i = 0; i < count; i++) {
        sum += accel_diff_window[i];
    }
    return sum / count;
}

// 更新运动状态
static void update_state(float avg_diff) {
    motion_state_t new_state;

    if (avg_diff < STATIC_THRESHOLD) {
        new_state = MOTION_STATIC;
    } else if (avg_diff < WALKING_THRESHOLD) {
        new_state = MOTION_WALKING;
    } else {
        new_state = MOTION_RUNNING;
    }

    // 只有状态真正改变时才更新
    if (new_state != current_state) {
        ESP_LOGI(TAG, "State changed: %d -> %d (avg_diff: %.3f)", current_state, new_state, avg_diff);
        current_state = new_state;
    }
}

// 更新卡路里消耗
static void update_calories(uint32_t now) {
    if (last_cal_update_time == 0) {
        last_cal_update_time = now;
        return;
    }

    uint32_t elapsed_ms = now - last_cal_update_time;
    if (elapsed_ms < CAL_UPDATE_INTERVAL) return;

    // 静止时不计算卡路里
    if (current_state == MOTION_STATIC) {
        last_cal_update_time = now;
        return;
    }

    float hours = (float)elapsed_ms / 3600000.0f;
    float met = 0;

    switch (current_state) {
        case MOTION_STATIC: met = 0; break;
        case MOTION_WALKING: met = CAL_WALKING_MET; break;
        case MOTION_RUNNING: met = CAL_RUNNING_MET; break;
    }

    // 卡路里 = MET × 体重(kg) × 时间(h)
    float calories = met * CAL_WEIGHT_KG * hours;
    total_calories += calories;

    last_cal_update_time = now;
}

void motion_detect_init(void) {
    current_state = MOTION_STATIC;
    window_index = 0;
    window_filled = false;
    total_calories = 0;
    last_cal_update_time = 0;
    prev_magnitude = 1.0f;  // 初始为重力加速度

    for (int i = 0; i < STATE_WINDOW_SIZE; i++) {
        accel_diff_window[i] = 0;
    }

    ESP_LOGI(TAG, "Motion detect initialized");
}

void motion_detect_update(float accel_x, float accel_y, float accel_z) {
    float magnitude = calc_magnitude(accel_x, accel_y, accel_z);

    // 计算加速度变化量（与上一次的差值）
    float diff = fabsf(magnitude - prev_magnitude);
    prev_magnitude = magnitude;

    // 更新滑动窗口
    accel_diff_window[window_index] = diff;
    window_index = (window_index + 1) % STATE_WINDOW_SIZE;
    if (window_index == 0) window_filled = true;

    // 计算平均变化量并更新状态
    float avg = calc_window_avg();
    update_state(avg);

    // 更新卡路里
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
    update_calories(now);
}

motion_state_t motion_detect_get_state(void) {
    return current_state;
}

const char* motion_detect_get_state_str(void) {
    switch (current_state) {
        case MOTION_STATIC: return "Static";
        case MOTION_WALKING: return "Walking";
        case MOTION_RUNNING: return "Running";
        default: return "Unknown";
    }
}

float motion_detect_get_calories(void) {
    return total_calories;
}

void motion_detect_reset(void) {
    total_calories = 0;
    last_cal_update_time = 0;
    ESP_LOGI(TAG, "Motion detect reset");
}

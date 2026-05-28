// IMU手势可视化/main/main.c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "mpu6050.h"

static const char *TAG = "MAIN";

void app_main(void) {
    ESP_LOGI(TAG, "IMU Gesture Visualizer - Phase 1");
    ESP_LOGI(TAG, "MPU6050 Serial Verification");

    // TODO: 后续步骤实现
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
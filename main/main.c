// main/main.c — Phase 2a: LVGL 显示角度
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "mpu6050.h"
#include "display.h"

static const char *TAG = "MAIN";

#define I2C_SDA_PIN  21
#define I2C_SCL_PIN  22

void app_main(void) {
    ESP_LOGI(TAG, "=== IMU Gesture Visualizer - Phase 2a ===");

    // 初始化显示
    esp_err_t ret = display_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize display");
        while (1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
    }

    // 初始化 I2C
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus_handle;
    ret = i2c_new_master_bus(&bus_config, &bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2C");
        while (1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
    }

    // 初始化 MPU6050
    mpu6050_data_t mpu_data;
    ret = mpu6050_init(bus_handle, &mpu_data);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize MPU6050");
        while (1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
    }

    ESP_LOGI(TAG, "Starting main loop...");

    while (1) {
        if (mpu6050_read(&mpu_data) == ESP_OK) {
            display_update(&mpu_data);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

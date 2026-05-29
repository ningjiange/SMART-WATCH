// main/main.c — Phase 4: LVGL + 3D 立方体 + WiFi + BLE
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "driver/i2c_master.h"
#include "mpu6050.h"
#include "display.h"
#include "web_server.h"

static const char *TAG = "MAIN";

#define I2C_SDA_PIN  32
#define I2C_SCL_PIN  33

// 全局传感器数据（互斥访问）
static mpu6050_data_t g_mpu_data;
static SemaphoreHandle_t g_data_mutex;

// MPU6050 任务（高优先级，独立 I2C 访问）
static void mpu_task(void *pvParameters) {
    esp_task_wdt_delete(NULL);

    i2c_master_bus_handle_t bus_handle = (i2c_master_bus_handle_t)pvParameters;
    vTaskDelay(pdMS_TO_TICKS(1000));

    mpu6050_data_t local_data;
    if (mpu6050_init(bus_handle, &local_data) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init MPU6050");
        vTaskDelete(NULL);
        return;
    }

    while (1) {
        if (mpu6050_read(&local_data) == ESP_OK) {
            if (xSemaphoreTake(g_data_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                g_mpu_data = local_data;
                xSemaphoreGive(g_data_mutex);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// WiFi 任务
static void wifi_task(void *pvParameters) {
    ESP_LOGI(TAG, "WiFi task started");
    web_server_init();
    vTaskDelete(NULL);
}

void app_main(void) {
    ESP_LOGI(TAG, "=== IMU Gesture Visualizer - Phase 3 ===");

    g_data_mutex = xSemaphoreCreateMutex();

    // 初始化显示
    esp_err_t ret = display_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize display");
        while (1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
    }

    // 初始化 I2C
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_1,
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

    // MPU6050 固定到 Core 1
    xTaskCreatePinnedToCore(mpu_task, "mpu_task", 4096, bus_handle, 5, NULL, 1);

    // WiFi 低优先级任务
    xTaskCreate(wifi_task, "wifi_task", 8192, NULL, 2, NULL);

    ESP_LOGI(TAG, "Starting display loop...");

    while (1) {
        if (xSemaphoreTake(g_data_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            display_update(&g_mpu_data);
            web_server_update_data(&g_mpu_data);
            xSemaphoreGive(g_data_mutex);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// main/main.c — Phase 5: UI 布局 + 传感器集成
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "driver/i2c_master.h"
#include "mpu6050.h"
#include "display.h"
#include "web_server.h"
#include "dht11.h"
#include "buzzer.h"
#include <math.h>

static const char *TAG = "MAIN";

#define I2C_SDA_PIN  32
#define I2C_SCL_PIN  33
#define DHT11_PIN    25
#define BUZZER_PIN   26

// 全局传感器数据（互斥访问）
static mpu6050_data_t g_mpu_data;
static sensor_display_data_t g_display_data;
static SemaphoreHandle_t g_data_mutex;

// 占位数据来源说明：
// - 温湿度：DHT11 (GPIO 25) 已集成，由 sensor_task 实时更新
// - 时间：NTP 网络同步未实现，暂用占位字符串
// - 天气：wttr.in API 未实现，暂用占位文本
// - IMU：MPU6050 已实现真实数据读取

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

// DHT11 传感器任务（读取温湿度）
static void sensor_task(void *pvParameters) {
    float temp, humi;
    while (1) {
        if (dht11_read(&temp, &humi) == ESP_OK) {
            if (xSemaphoreTake(g_data_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                g_display_data.temperature = temp;
                g_display_data.humidity = humi;
                xSemaphoreGive(g_data_mutex);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

// 蜂鸣器报警任务
static void buzzer_task(void *pvParameters) {
    while (1) {
        if (xSemaphoreTake(g_data_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            if (fabs(g_display_data.pitch) > 30 || fabs(g_display_data.roll) > 30) {
                buzzer_short_beep();
            }
            if (g_display_data.temperature > 35) {
                buzzer_long_beep();
            }
            xSemaphoreGive(g_data_mutex);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// WiFi 任务
static void wifi_task(void *pvParameters) {
    ESP_LOGI(TAG, "WiFi task started");
    web_server_init();
    vTaskDelete(NULL);
}

void app_main(void) {
    ESP_LOGI(TAG, "=== IMU Gesture Visualizer - Phase 5 ===");

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

    // 初始化 DHT11 和蜂鸣器
    dht11_init(DHT11_PIN);
    buzzer_init(BUZZER_PIN);

    // 启动传感器和蜂鸣器任务
    xTaskCreate(sensor_task, "sensor_task", 4096, NULL, 4, NULL);
    xTaskCreate(buzzer_task, "buzzer_task", 2048, NULL, 3, NULL);

    ESP_LOGI(TAG, "Starting display loop...");

    while (1) {
        if (xSemaphoreTake(g_data_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            // 填充显示数据
            g_display_data.pitch = g_mpu_data.pitch;
            g_display_data.roll = g_mpu_data.roll;

            // === 占位数据（后续替换为真实数据源） ===
            // DHT11 温湿度：由 sensor_task 实时更新

            // NTP 时间（驱动未实现）
            snprintf(g_display_data.time_str, sizeof(g_display_data.time_str), "12:34");

            // wttr.in 天气（驱动未实现）
            snprintf(g_display_data.weather, sizeof(g_display_data.weather), "Weather: --");

            // WiFi 状态
            snprintf(g_display_data.wifi_status, sizeof(g_display_data.wifi_status), "Connected");

            xSemaphoreGive(g_data_mutex);

            display_update(&g_display_data);
            web_server_update_data(&g_mpu_data);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

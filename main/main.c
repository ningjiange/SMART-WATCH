// main/main.c — Phase 5: 多页面 UI + 传感器 + 秒表/倒计时
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
#include "input.h"
#include "page_manager.h"
#include "stopwatch.h"
#include "countdown.h"
#include "pedometer.h"
#include "motion_detect.h"
#include "flashlight.h"
#include "ntp_sync.h"
#include "weather.h"
#include "system_info.h"
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

// 工具页面状态
static uint8_t tool_mode = 0;  // 0=秒表，1=倒计时

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
            // 更新计步器和运动检测
            pedometer_update(local_data.accel_x, local_data.accel_y, local_data.accel_z);
            motion_detect_update(local_data.accel_x, local_data.accel_y, local_data.accel_z);

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
        vTaskDelay(pdMS_TO_TICKS(1000));
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

            // 倒计时结束时蜂鸣
            if (countdown_is_finished()) {
                buzzer_long_beep();
                countdown_reset();
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

    // 初始化 NTP 时间同步
    ntp_sync_init();

    // 初始化天气模块
    weather_init();

    // 等待 WiFi 连接后更新天气
    vTaskDelay(pdMS_TO_TICKS(3000));
    weather_update();

    vTaskDelete(NULL);
}

// 按键检测任务（高优先级，快速响应）
static void button_task(void *pvParameters) {
    ESP_LOGI(TAG, "Button task started");
    while (1) {
        input_event_t event = input_read();
        page_id_t current = page_manager_get_current();

        if (event != INPUT_NONE) {
            ESP_LOGI(TAG, "Event: %d, Page: %d, ToolMode: %d", event, current, tool_mode);
        }

        if (event == INPUT_UP) {
            if (current == PAGE_TOOLS) {
                if (tool_mode == 1) {
                    // 倒计时选中：UP 切换到秒表
                    tool_mode = 0;
                    ESP_LOGI(TAG, "Tool: Stopwatch");
                } else {
                    // 秒表选中：UP 切换到上一页
                    page_manager_prev();
                    ESP_LOGI(TAG, "Page: %d", page_manager_get_current());
                }
            } else {
                page_manager_prev();
                ESP_LOGI(TAG, "Page: %d", page_manager_get_current());
            }
            vTaskDelay(pdMS_TO_TICKS(300));
        } else if (event == INPUT_DOWN) {
            if (current == PAGE_TOOLS) {
                if (tool_mode == 0) {
                    // 秒表选中：DOWN 切换到倒计时
                    tool_mode = 1;
                    ESP_LOGI(TAG, "Tool: Countdown");
                } else {
                    // 倒计时选中：DOWN 切换到下一页
                    page_manager_next();
                    ESP_LOGI(TAG, "Page: %d", page_manager_get_current());
                }
            } else {
                page_manager_next();
                ESP_LOGI(TAG, "Page: %d", page_manager_get_current());
            }
            vTaskDelay(pdMS_TO_TICKS(300));
        } else if (event == INPUT_SELECT) {
            if (current == PAGE_TOOLS) {
                // 工具页面：SELECT 启动/停止当前工具
                if (tool_mode == 0) {
                    // 秒表模式
                    if (stopwatch_is_running()) {
                        stopwatch_stop();
                        ESP_LOGI(TAG, "Stopwatch stopped");
                    } else {
                        if (stopwatch_get_ms() == 0) {
                            stopwatch_reset();
                        }
                        stopwatch_start();
                        ESP_LOGI(TAG, "Stopwatch started");
                    }
                } else {
                    // 倒计时模式
                    if (countdown_is_running()) {
                        countdown_stop();
                        ESP_LOGI(TAG, "Countdown stopped");
                    } else {
                        countdown_start();
                        ESP_LOGI(TAG, "Countdown started");
                    }
                }
            } else if (current == PAGE_GAME) {
                // 手电筒页面：切换开关
                flashlight_toggle();
                ESP_LOGI(TAG, "Flashlight toggled");
            } else {
                page_manager_select();
            }
            vTaskDelay(pdMS_TO_TICKS(300));
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// 工具页面长按切换模式（通过 UP+SELECT 组合）
static void tools_mode_switch(void) {
    if (page_manager_get_current() == PAGE_TOOLS) {
        tool_mode = (tool_mode + 1) % 2;
        ESP_LOGI(TAG, "Tool mode: %s", tool_mode == 0 ? "Stopwatch" : "Countdown");
    }
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

    // 初始化秒表和倒计时
    stopwatch_init();
    countdown_init();

    // 初始化计步器和运动检测
    pedometer_init();
    motion_detect_init();

    // 初始化手电筒
    flashlight_init();

    // 初始化系统信息
    system_info_init();

    // 启动传感器和蜂鸣器任务
    xTaskCreate(sensor_task, "sensor_task", 4096, NULL, 4, NULL);
    xTaskCreate(buzzer_task, "buzzer_task", 2048, NULL, 3, NULL);

    // 初始化输入设备和页面管理器
    input_init();
    page_manager_init();

    // 按键检测任务（高优先级）
    xTaskCreate(button_task, "button_task", 2048, NULL, 6, NULL);

    ESP_LOGI(TAG, "Starting display loop...");

    while (1) {
        // 更新秒表和倒计时
        stopwatch_update();
        countdown_update();


        if (xSemaphoreTake(g_data_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            // 填充显示数据
            g_display_data.pitch = g_mpu_data.pitch;
            g_display_data.roll = g_mpu_data.roll;

            // NTP 时间
            ntp_sync_get_time_str(g_display_data.time_str, sizeof(g_display_data.time_str));

            // 天气信息
            weather_get_info(g_display_data.weather, sizeof(g_display_data.weather));

            // WiFi 状态
            snprintf(g_display_data.wifi_status, sizeof(g_display_data.wifi_status), "Connected");

            // 秒表时间
            stopwatch_get_time(g_display_data.stopwatch_str, sizeof(g_display_data.stopwatch_str));

            // 倒计时时间
            countdown_get_time(g_display_data.countdown_str, sizeof(g_display_data.countdown_str));

            // 工具模式
            g_display_data.tool_mode = tool_mode;

            // 手电筒状态
            g_display_data.flashlight_on = flashlight_is_on();

            // 运动数据
            g_display_data.steps = pedometer_get_steps();
            g_display_data.motion_state = motion_detect_get_state();
            g_display_data.calories = motion_detect_get_calories();

            xSemaphoreGive(g_data_mutex);

            display_update(&g_display_data);
            web_server_update_data(&g_mpu_data);
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

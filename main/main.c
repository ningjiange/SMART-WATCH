#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "mpu6050.h"

static const char *TAG = "MAIN";

#define I2C_SDA_PIN  21
#define I2C_SCL_PIN  22
#define I2C_FREQ_HZ  400000

void app_main(void) {
    ESP_LOGI(TAG, "=== IMU Gesture Visualizer - Phase 1 ===");
    ESP_LOGI(TAG, "MPU6050 Serial Verification");

    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));
    ESP_LOGI(TAG, "I2C initialized: SDA=%d, SCL=%d", I2C_SDA_PIN, I2C_SCL_PIN);

    mpu6050_data_t mpu_data;
    ESP_ERROR_CHECK(mpu6050_init(bus_handle, &mpu_data));

    ESP_LOGI(TAG, "Starting data read loop...");
    ESP_LOGI(TAG, "%-8s %-8s %-8s | %-8s %-8s %-8s | %-8s %-8s",
             "Accel_X", "Accel_Y", "Accel_Z", "Gyro_X", "Gyro_Y", "Gyro_Z", "Pitch", "Roll");
    ESP_LOGI(TAG, "-------- -------- -------- | -------- -------- -------- | -------- --------");

    while (1) {
        if (mpu6050_read(&mpu_data) == ESP_OK) {
            ESP_LOGI(TAG, "%7.2f  %7.2f  %7.2f | %7.2f  %7.2f  %7.2f | %6.1f°  %6.1f°",
                     mpu_data.accel_x, mpu_data.accel_y, mpu_data.accel_z,
                     mpu_data.gyro_x, mpu_data.gyro_y, mpu_data.gyro_z,
                     mpu_data.pitch, mpu_data.roll);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

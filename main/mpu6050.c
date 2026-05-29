#include "mpu6050.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>

static const char *TAG = "MPU6050";
static i2c_master_dev_handle_t dev_handle = NULL;

static esp_err_t mpu6050_write_reg(uint8_t reg, uint8_t value) {
    uint8_t buf[2] = {reg, value};
    return i2c_master_transmit(dev_handle, buf, 2, 100);
}

static esp_err_t mpu6050_read_regs(uint8_t reg, uint8_t *data, size_t len) {
    return i2c_master_transmit_receive(dev_handle, &reg, 1, data, len, 100);
}

static esp_err_t mpu6050_read_reg(uint8_t reg, uint8_t *value) {
    return mpu6050_read_regs(reg, value, 1);
}

static int16_t combine_bytes(uint8_t high, uint8_t low) {
    return (int16_t)((high << 8) | low);
}

esp_err_t mpu6050_init(i2c_master_bus_handle_t bus_handle, mpu6050_data_t *data) {
    ESP_LOGI(TAG, "Initializing MPU6050...");

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = MPU6050_ADDR,
        .scl_speed_hz = 400000,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

    uint8_t who_am_i = 0;
    esp_err_t ret = mpu6050_read_reg(MPU6050_REG_WHO_AM_I, &who_am_i);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read WHO_AM_I: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "WHO_AM_I = 0x%02X", who_am_i);
    if (who_am_i != 0x68 && who_am_i != 0x70) {
        ESP_LOGE(TAG, "Invalid MPU6050 device! Got 0x%02X", who_am_i);
        return ESP_ERR_INVALID_RESPONSE;
    }

    ESP_ERROR_CHECK(mpu6050_write_reg(MPU6050_REG_PWR_MGMT_1, 0x00));
    vTaskDelay(pdMS_TO_TICKS(100));

    ESP_ERROR_CHECK(mpu6050_write_reg(MPU6050_REG_SMPLRT_DIV, 0x09));
    ESP_ERROR_CHECK(mpu6050_write_reg(MPU6050_REG_CONFIG, 0x03));
    ESP_ERROR_CHECK(mpu6050_write_reg(MPU6050_REG_ACCEL_CONFIG, 0x00));
    ESP_ERROR_CHECK(mpu6050_write_reg(MPU6050_REG_GYRO_CONFIG, 0x00));

    ESP_LOGI(TAG, "MPU6050 initialized successfully");
    return ESP_OK;
}

esp_err_t mpu6050_read(mpu6050_data_t *data) {
    uint8_t buf[14];
    esp_err_t ret = mpu6050_read_regs(MPU6050_REG_ACCEL_XOUT_H, buf, 14);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read sensor data: %s", esp_err_to_name(ret));
        return ret;
    }

    int16_t raw_ax = combine_bytes(buf[0], buf[1]);
    int16_t raw_ay = combine_bytes(buf[2], buf[3]);
    int16_t raw_az = combine_bytes(buf[4], buf[5]);
    data->accel_x = raw_ax * 9.81f / 16384.0f;
    data->accel_y = raw_ay * 9.81f / 16384.0f;
    data->accel_z = raw_az * 9.81f / 16384.0f;

    int16_t raw_gx = combine_bytes(buf[8], buf[9]);
    int16_t raw_gy = combine_bytes(buf[10], buf[11]);
    int16_t raw_gz = combine_bytes(buf[12], buf[13]);
    data->gyro_x = raw_gx * 250.0f / 32768.0f;
    data->gyro_y = raw_gy * 250.0f / 32768.0f;
    data->gyro_z = raw_gz * 250.0f / 32768.0f;

    data->pitch = atan2f(data->accel_x, sqrtf(data->accel_y * data->accel_y + data->accel_z * data->accel_z)) * 180.0f / M_PI;
    data->roll = atan2f(data->accel_y, sqrtf(data->accel_x * data->accel_x + data->accel_z * data->accel_z)) * 180.0f / M_PI;

    return ESP_OK;
}

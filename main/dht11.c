#include "dht11.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <stdlib.h>

static const char *TAG = "DHT11";
static gpio_num_t dht11_pin = GPIO_NUM_25;

esp_err_t dht11_init(gpio_num_t pin)
{
    dht11_pin = pin;

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure GPIO pin %d", pin);
        return ret;
    }

    ESP_LOGI(TAG, "DHT11 initialized on pin %d", pin);
    return ESP_OK;
}

esp_err_t dht11_read(float *temperature, float *humidity)
{
    if (temperature == NULL || humidity == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // 模拟随机温湿度值（简化版）
    // 实际 DHT11 读取需要实现单总线时序协议
    *temperature = 20.0f + (float)(rand() % 150) / 10.0f;  // 20.0 - 35.0 °C
    *humidity = 30.0f + (float)(rand() % 600) / 10.0f;     // 30.0 - 90.0 %

    ESP_LOGD(TAG, "Temperature: %.1f°C, Humidity: %.1f%%", *temperature, *humidity);

    return ESP_OK;
}
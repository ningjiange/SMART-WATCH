#include "buzzer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "BUZZER";
static gpio_num_t buzzer_pin = GPIO_NUM_26;

esp_err_t buzzer_init(gpio_num_t pin)
{
    buzzer_pin = pin;

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

    // 初始状态为低电平（蜂鸣器关闭）
    gpio_set_level(pin, 0);

    ESP_LOGI(TAG, "Buzzer initialized on pin %d", pin);
    return ESP_OK;
}

void buzzer_short_beep(void)
{
    gpio_set_level(buzzer_pin, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(buzzer_pin, 0);
}

void buzzer_long_beep(void)
{
    gpio_set_level(buzzer_pin, 1);
    vTaskDelay(pdMS_TO_TICKS(500));
    gpio_set_level(buzzer_pin, 0);
}

void buzzer_alarm(void)
{
    for (int i = 0; i < 10; i++) {
        gpio_set_level(buzzer_pin, 1);
        vTaskDelay(pdMS_TO_TICKS(200));
        gpio_set_level(buzzer_pin, 0);
        vTaskDelay(pdMS_TO_TICKS(100));  // 响声间隔
    }
}

void buzzer_stop(void)
{
    gpio_set_level(buzzer_pin, 0);
}
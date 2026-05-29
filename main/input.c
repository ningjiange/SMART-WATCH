#include "input.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdbool.h>

// 按键 GPIO 配置 (低电平有效)
#define GPIO_INPUT_UP      12
#define GPIO_INPUT_DOWN    14
#define GPIO_INPUT_SELECT  27

#define DEBOUNCE_MS        20

static bool input_initialized = false;

esp_err_t input_init(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_INPUT_UP) |
                        (1ULL << GPIO_INPUT_DOWN) |
                        (1ULL << GPIO_INPUT_SELECT),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) return ret;
    input_initialized = true;
    return ESP_OK;
}

input_event_t input_read(void) {
    if (!input_initialized) return INPUT_NONE;

    // 检测按下（低电平有效）
    if (gpio_get_level(GPIO_INPUT_UP) == 0) {
        vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
        if (gpio_get_level(GPIO_INPUT_UP) == 0) return INPUT_UP;
    }
    if (gpio_get_level(GPIO_INPUT_DOWN) == 0) {
        vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
        if (gpio_get_level(GPIO_INPUT_DOWN) == 0) return INPUT_DOWN;
    }
    if (gpio_get_level(GPIO_INPUT_SELECT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
        if (gpio_get_level(GPIO_INPUT_SELECT) == 0) return INPUT_SELECT;
    }
    return INPUT_NONE;
}

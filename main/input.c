#include "input.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdbool.h>

// 按键 GPIO 配置 (低电平有效)
#define GPIO_INPUT_UP      5
#define GPIO_INPUT_DOWN    13
#define GPIO_INPUT_SELECT  27

#define DEBOUNCE_MS        20

static bool input_initialized = false;

// 按键状态跟踪（用于边沿检测）
static bool last_up_state = true;
static bool last_down_state = true;
static bool last_select_state = true;

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

    // 初始化状态
    last_up_state = gpio_get_level(GPIO_INPUT_UP);
    last_down_state = gpio_get_level(GPIO_INPUT_DOWN);
    last_select_state = gpio_get_level(GPIO_INPUT_SELECT);

    input_initialized = true;
    return ESP_OK;
}

input_event_t input_read(void) {
    if (!input_initialized) return INPUT_NONE;

    bool current_up = gpio_get_level(GPIO_INPUT_UP);
    bool current_down = gpio_get_level(GPIO_INPUT_DOWN);
    bool current_select = gpio_get_level(GPIO_INPUT_SELECT);

    input_event_t event = INPUT_NONE;

    // 边沿检测：只在按下瞬间触发（从高变低）
    if (current_up == 0 && last_up_state == 1) {
        vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
        if (gpio_get_level(GPIO_INPUT_UP) == 0) {
            event = INPUT_UP;
        }
    }

    if (event == INPUT_NONE && current_down == 0 && last_down_state == 1) {
        vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
        if (gpio_get_level(GPIO_INPUT_DOWN) == 0) {
            event = INPUT_DOWN;
        }
    }

    if (event == INPUT_NONE && current_select == 0 && last_select_state == 1) {
        vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
        if (gpio_get_level(GPIO_INPUT_SELECT) == 0) {
            event = INPUT_SELECT;
        }
    }

    // 更新状态（无论是否检测到事件）
    last_up_state = current_up;
    last_down_state = current_down;
    last_select_state = current_select;

    return event;
}

#include "input.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdbool.h>

// 按键 GPIO 配置 (低电平有效)
#define GPIO_INPUT_UP      34
#define GPIO_INPUT_DOWN    35
#define GPIO_INPUT_SELECT  13

#define DEBOUNCE_MS        50

static bool input_initialized = false;

esp_err_t input_init(void) {
    // 配置 GPIO 为输入模式，上拉电阻
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
    if (ret != ESP_OK) {
        return ret;
    }

    input_initialized = true;
    return ESP_OK;
}

input_event_t input_read(void) {
    if (!input_initialized) {
        return INPUT_NONE;
    }

    // 读取按键状态 (低电平有效)
    bool up_pressed = !gpio_get_level(GPIO_INPUT_UP);
    bool down_pressed = !gpio_get_level(GPIO_INPUT_DOWN);
    bool select_pressed = !gpio_get_level(GPIO_INPUT_SELECT);

    // 没有按键按下
    if (!up_pressed && !down_pressed && !select_pressed) {
        return INPUT_NONE;
    }

    // 消抖延迟
    vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));

    // 再次确认按键状态
    up_pressed = !gpio_get_level(GPIO_INPUT_UP);
    down_pressed = !gpio_get_level(GPIO_INPUT_DOWN);
    select_pressed = !gpio_get_level(GPIO_INPUT_SELECT);

    input_event_t event = INPUT_NONE;

    // 确定按下的是哪个按键 (优先级: SELECT > UP > DOWN)
    if (select_pressed) {
        event = INPUT_SELECT;
    } else if (up_pressed) {
        event = INPUT_UP;
    } else if (down_pressed) {
        event = INPUT_DOWN;
    }

    // 等待按键释放
    if (event != INPUT_NONE) {
        while (!gpio_get_level(GPIO_INPUT_UP) ||
               !gpio_get_level(GPIO_INPUT_DOWN) ||
               !gpio_get_level(GPIO_INPUT_SELECT)) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
    }

    return event;
}

// main/flashlight.c — 手电筒功能
#include "flashlight.h"
#include "esp_log.h"
#include "driver/gpio.h"

static const char *TAG = "FLASHLIGHT";

// 手电筒控制的 GPIO（空闲引脚）
#define FLASHLIGHT_GPIO    14  // GPIO 14 空闲，可用于外部 LED

static bool flashlight_on = false;

void flashlight_init(void) {
    // 初始化 GPIO 为输出模式
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << FLASHLIGHT_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level(FLASHLIGHT_GPIO, 0);

    ESP_LOGI(TAG, "Flashlight initialized (GPIO %d)", FLASHLIGHT_GPIO);
}

void flashlight_toggle(void) {
    flashlight_on = !flashlight_on;
    gpio_set_level(FLASHLIGHT_GPIO, flashlight_on ? 1 : 0);
    ESP_LOGI(TAG, "Flashlight: %s", flashlight_on ? "ON" : "OFF");
}

bool flashlight_is_on(void) {
    return flashlight_on;
}

#include "touch.h"
#include <stdbool.h>

static bool touch_initialized = false;

esp_err_t touch_init(void) {
    // 占位实现 - XPT2046 引脚与 MPU6050 冲突，暂时不初始化
    touch_initialized = true;
    return ESP_OK;
}

bool touch_read(touch_event_t *event) {
    if (!touch_initialized || !event) {
        return false;
    }

    // 占位实现 - 返回无触摸状态
    event->x = 0;
    event->y = 0;
    event->pressed = false;

    return false;
}

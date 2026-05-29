// main/ble_control.h
#ifndef BLE_CONTROL_H
#define BLE_CONTROL_H

#include "esp_err.h"

// BLE 控制模式
typedef enum {
    MODE_FREE = 0,   // 自由旋转
    MODE_LOCK = 1,   // 锁定角度
} ble_mode_t;

// 初始化 BLE
esp_err_t ble_control_init(void);

// 获取当前模式
ble_mode_t ble_control_get_mode(void);

// 获取报警阈值
int ble_control_get_threshold(void);

#endif

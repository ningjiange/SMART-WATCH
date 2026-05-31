// main/pedometer.h
#ifndef PEDOMETER_H
#define PEDOMETER_H

#include <stdbool.h>
#include <stdint.h>

// 初始化计步器
void pedometer_init(void);

// 更新计步器（传入加速度数据）
void pedometer_update(float accel_x, float accel_y, float accel_z);

// 获取总步数
uint32_t pedometer_get_steps(void);

// 重置步数
void pedometer_reset(void);

// 获取当前加速度幅值
float pedometer_get_magnitude(void);

#endif // PEDOMETER_H

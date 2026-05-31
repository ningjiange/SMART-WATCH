// main/motion_detect.h
#ifndef MOTION_DETECT_H
#define MOTION_DETECT_H

#include <stdint.h>
#include <stdbool.h>

// 运动状态枚举
typedef enum {
    MOTION_STATIC = 0,   // 静止
    MOTION_WALKING = 1,  // 走路
    MOTION_RUNNING = 2   // 跑步
} motion_state_t;

// 初始化运动检测
void motion_detect_init(void);

// 更新运动检测（传入加速度数据）
void motion_detect_update(float accel_x, float accel_y, float accel_z);

// 获取当前运动状态
motion_state_t motion_detect_get_state(void);

// 获取运动状态字符串
const char* motion_detect_get_state_str(void);

// 获取卡路里消耗（kcal）
float motion_detect_get_calories(void);

// 重置运动数据
void motion_detect_reset(void);

#endif // MOTION_DETECT_H

// main/flashlight.h
#ifndef FLASHLIGHT_H
#define FLASHLIGHT_H

#include <stdbool.h>

// 初始化手电筒
void flashlight_init(void);

// 切换手电筒开关状态
void flashlight_toggle(void);

// 获取手电筒当前状态
bool flashlight_is_on(void);

#endif // FLASHLIGHT_H

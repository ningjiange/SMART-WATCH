// main/weather.h
#ifndef WEATHER_H
#define WEATHER_H

#include <stdbool.h>
#include <stddef.h>

// 初始化天气模块
void weather_init(void);

// 更新天气信息（从网络获取）
void weather_update(void);

// 获取天气信息字符串
bool weather_get_info(char *buf, size_t len);

#endif // WEATHER_H

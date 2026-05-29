// main/web_server.h
#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "esp_err.h"
#include "mpu6050.h"

// 初始化 WiFi 和 HTTP Server
esp_err_t web_server_init(void);

// 更新传感器数据（供 HTTP 请求使用）
void web_server_update_data(const mpu6050_data_t *data);

#endif

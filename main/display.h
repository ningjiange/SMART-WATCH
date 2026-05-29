// main/display.h
#ifndef DISPLAY_H
#define DISPLAY_H

#include "esp_err.h"
#include "mpu6050.h"

esp_err_t display_init(void);
void display_update(const mpu6050_data_t *data);

#endif

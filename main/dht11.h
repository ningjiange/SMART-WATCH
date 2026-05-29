#ifndef DHT11_H
#define DHT11_H

#include "esp_err.h"
#include "driver/gpio.h"

esp_err_t dht11_init(gpio_num_t pin);
esp_err_t dht11_read(float *temperature, float *humidity);

#endif
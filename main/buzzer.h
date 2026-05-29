#ifndef BUZZER_H
#define BUZZER_H

#include "esp_err.h"
#include "driver/gpio.h"

esp_err_t buzzer_init(gpio_num_t pin);
void buzzer_short_beep(void);
void buzzer_long_beep(void);
void buzzer_alarm(void);
void buzzer_stop(void);

#endif
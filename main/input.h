#ifndef INPUT_H
#define INPUT_H

#include "esp_err.h"

typedef enum {
    INPUT_NONE,
    INPUT_UP,
    INPUT_DOWN,
    INPUT_SELECT
} input_event_t;

esp_err_t input_init(void);
input_event_t input_read(void);

#endif // INPUT_H

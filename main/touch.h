#ifndef TOUCH_H
#define TOUCH_H

#include "esp_err.h"
#include <stdbool.h>

typedef struct {
    int x;
    int y;
    bool pressed;
} touch_event_t;

esp_err_t touch_init(void);
bool touch_read(touch_event_t *event);

#endif // TOUCH_H

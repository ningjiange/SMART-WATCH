// main/countdown.h
#ifndef COUNTDOWN_H
#define COUNTDOWN_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

void countdown_init(void);
void countdown_set(uint32_t seconds);
void countdown_start(void);
void countdown_stop(void);
void countdown_reset(void);
void countdown_update(void);
bool countdown_is_running(void);
bool countdown_is_finished(void);
uint32_t countdown_get_remaining_ms(void);
void countdown_get_time(char *buf, size_t len);

#endif // COUNTDOWN_H

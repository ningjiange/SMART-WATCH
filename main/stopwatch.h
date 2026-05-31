// main/stopwatch.h
#ifndef STOPWATCH_H
#define STOPWATCH_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

void stopwatch_init(void);
void stopwatch_start(void);
void stopwatch_stop(void);
void stopwatch_reset(void);
void stopwatch_update(void);
bool stopwatch_is_running(void);
uint32_t stopwatch_get_ms(void);
void stopwatch_get_time(char *buf, size_t len);

#endif // STOPWATCH_H

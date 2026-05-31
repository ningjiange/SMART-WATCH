// main/stopwatch.c — 秒表功能
#include "stopwatch.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <stdio.h>

static const char *TAG = "STOPWATCH";

static bool running = false;
static uint32_t elapsed_ms = 0;
static uint32_t last_tick_ms = 0;

void stopwatch_init(void) {
    running = false;
    elapsed_ms = 0;
    last_tick_ms = 0;
    ESP_LOGI(TAG, "Stopwatch initialized");
}

void stopwatch_start(void) {
    if (!running) {
        running = true;
        last_tick_ms = esp_timer_get_time() / 1000;
        ESP_LOGI(TAG, "Stopwatch started");
    }
}

void stopwatch_stop(void) {
    if (running) {
        running = false;
        ESP_LOGI(TAG, "Stopwatch stopped");
    }
}

void stopwatch_reset(void) {
    running = false;
    elapsed_ms = 0;
    ESP_LOGI(TAG, "Stopwatch reset");
}

void stopwatch_update(void) {
    if (running) {
        uint32_t now = esp_timer_get_time() / 1000;
        elapsed_ms += (now - last_tick_ms);
        last_tick_ms = now;
    }
}

bool stopwatch_is_running(void) {
    return running;
}

uint32_t stopwatch_get_ms(void) {
    return elapsed_ms;
}

void stopwatch_get_time(char *buf, size_t len) {
    uint32_t total_sec = elapsed_ms / 1000;
    uint32_t ms = elapsed_ms % 1000;
    uint32_t min = total_sec / 60;
    uint32_t sec = total_sec % 60;

    snprintf(buf, len, "%02lu:%02lu.%03lu", min, sec, ms);
}

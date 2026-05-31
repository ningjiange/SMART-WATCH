// main/countdown.c — 倒计时功能
#include "countdown.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <stdio.h>

static const char *TAG = "COUNTDOWN";

static bool running = false;
static uint32_t remaining_ms = 60000;  // 默认 60 秒
static uint32_t last_tick_ms = 0;
static bool finished = false;

void countdown_init(void) {
    running = false;
    remaining_ms = 60000;
    last_tick_ms = 0;
    finished = false;
    ESP_LOGI(TAG, "Countdown initialized (60s)");
}

void countdown_set(uint32_t seconds) {
    if (!running) {
        remaining_ms = seconds * 1000;
        finished = false;
        ESP_LOGI(TAG, "Countdown set to %lu seconds", seconds);
    }
}

void countdown_start(void) {
    if (!running && remaining_ms > 0) {
        running = true;
        finished = false;
        last_tick_ms = esp_timer_get_time() / 1000;
        ESP_LOGI(TAG, "Countdown started");
    }
}

void countdown_stop(void) {
    if (running) {
        running = false;
        ESP_LOGI(TAG, "Countdown stopped");
    }
}

void countdown_reset(void) {
    running = false;
    remaining_ms = 60000;
    finished = false;
    ESP_LOGI(TAG, "Countdown reset");
}

void countdown_update(void) {
    if (running && remaining_ms > 0) {
        uint32_t now = esp_timer_get_time() / 1000;
        uint32_t elapsed = now - last_tick_ms;

        if (elapsed >= remaining_ms) {
            remaining_ms = 0;
            running = false;
            finished = true;
            ESP_LOGI(TAG, "Countdown finished!");
        } else {
            remaining_ms -= elapsed;
        }
        last_tick_ms = now;
    }
}

bool countdown_is_running(void) {
    return running;
}

bool countdown_is_finished(void) {
    return finished;
}

uint32_t countdown_get_remaining_ms(void) {
    return remaining_ms;
}

void countdown_get_time(char *buf, size_t len) {
    uint32_t total_sec = remaining_ms / 1000;
    uint32_t ms = remaining_ms % 1000;
    uint32_t min = total_sec / 60;
    uint32_t sec = total_sec % 60;

    snprintf(buf, len, "%02lu:%02lu.%03lu", min, sec, ms);
}

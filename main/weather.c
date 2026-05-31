// main/weather.c — 天气 API (wttr.in)
#include "weather.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "WEATHER";

// wttr.in API 配置
#define WEATHER_URL "http://wttr.in/?format=%t+%C&lang=zh"

static char weather_info[64] = "Weather: --";
static bool weather_updated = false;

// HTTP 事件处理
static esp_err_t weather_http_event_handler(esp_http_client_event_t *evt) {
    switch (evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGE(TAG, "HTTP error");
            break;
        case HTTP_EVENT_ON_DATA:
            if (evt->data_len > 0 && evt->data) {
                // 解析天气数据
                char *data = (char *)evt->data;
                int len = evt->data_len;

                // 去除换行符
                while (len > 0 && (data[len-1] == '\n' || data[len-1] == '\r')) {
                    len--;
                }

                if (len > 0 && len < sizeof(weather_info) - 10) {
                    snprintf(weather_info, sizeof(weather_info), "%.*s", len, data);
                    weather_updated = true;
                    ESP_LOGI(TAG, "Weather: %s", weather_info);
                }
            }
            break;
        case HTTP_EVENT_DISCONNECTED:
            break;
        default:
            break;
    }
    return ESP_OK;
}

void weather_init(void) {
    ESP_LOGI(TAG, "Weather module initialized");
}

void weather_update(void) {
    ESP_LOGI(TAG, "Fetching weather...");

    esp_http_client_config_t config = {
        .url = WEATHER_URL,
        .event_handler = weather_http_event_handler,
        .timeout_ms = 5000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to init HTTP client");
        return;
    }

    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_close(client);
    esp_http_client_cleanup(client);
}

bool weather_get_info(char *buf, size_t len) {
    if (weather_updated) {
        strncpy(buf, weather_info, len);
        return true;
    } else {
        strncpy(buf, "Weather: --", len);
        return false;
    }
}

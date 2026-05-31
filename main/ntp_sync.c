// main/ntp_sync.c — NTP 时间同步
#include "ntp_sync.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include <time.h>
#include <string.h>

static const char *TAG = "NTP_SYNC";

// NTP 服务器配置
#define NTP_SERVER "ntp.aliyun.com"
#define TZ_INFO "CST-8"  // 中国标准时间 UTC+8

static bool time_synced = false;

// SNTP 回调函数
static void time_sync_cb(struct timeval *tv) {
    ESP_LOGI(TAG, "Time synchronized");
    time_synced = true;
}

void ntp_sync_init(void) {
    ESP_LOGI(TAG, "Initializing NTP sync...");

    // 设置时区
    setenv("TZ", TZ_INFO, 1);
    tzset();

    // 初始化 SNTP
    esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, NTP_SERVER);
    esp_sntp_set_time_sync_notification_cb(time_sync_cb);
    esp_sntp_init();

    ESP_LOGI(TAG, "NTP sync started, server: %s", NTP_SERVER);
}

bool ntp_sync_is_time_synced(void) {
    return time_synced;
}

void ntp_sync_get_time_str(char *buf, size_t len) {
    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);

    if (time_synced && timeinfo.tm_year > (2016 - 1900)) {
        // 时间已同步，格式化为 HH:MM
        strftime(buf, len, "%H:%M", &timeinfo);
    } else {
        // 时间未同步，显示占位符
        strncpy(buf, "--:--", len);
    }
}

void ntp_sync_get_datetime_str(char *buf, size_t len) {
    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);

    if (time_synced && timeinfo.tm_year > (2016 - 1900)) {
        // 格式化为 YYYY-MM-DD HH:MM:SS
        strftime(buf, len, "%Y-%m-%d %H:%M:%S", &timeinfo);
    } else {
        strncpy(buf, "----/--/-- --:--:--", len);
    }
}

// main/ntp_sync.h
#ifndef NTP_SYNC_H
#define NTP_SYNC_H

#include <stdbool.h>
#include <stddef.h>

// 初始化 NTP 时间同步
void ntp_sync_init(void);

// 检查时间是否已同步
bool ntp_sync_is_time_synced(void);

// 获取时间字符串（HH:MM 格式）
void ntp_sync_get_time_str(char *buf, size_t len);

// 获取日期时间字符串（YYYY-MM-DD HH:MM:SS 格式）
void ntp_sync_get_datetime_str(char *buf, size_t len);

#endif // NTP_SYNC_H

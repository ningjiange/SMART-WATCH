// main/ble_control.c — BLE NimBLE GATT Server
#include "ble_control.h"
#include "esp_log.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include <string.h>

static const char *TAG = "BLE_CONTROL";

#define DEVICE_NAME     "IMU-Visualizer"
#define SVC_UUID        0xFF00
#define CHAR_UUID       0xFF01

static ble_mode_t current_mode = MODE_FREE;
static int alarm_threshold = 30;
static uint16_t conn_handle = 0;

// 处理接收到的指令
static void process_command(const uint8_t *data, uint16_t len) {
    char cmd[64] = {0};
    int copy_len = len < (int)(sizeof(cmd) - 1) ? len : (int)(sizeof(cmd) - 1);
    memcpy(cmd, data, copy_len);

    ESP_LOGI(TAG, "Received: %s", cmd);

    if (strncmp(cmd, "MODE:FREE", 9) == 0) {
        current_mode = MODE_FREE;
        ESP_LOGI(TAG, "Mode: FREE");
    } else if (strncmp(cmd, "MODE:LOCK", 9) == 0) {
        current_mode = MODE_LOCK;
        ESP_LOGI(TAG, "Mode: LOCK");
    } else if (strncmp(cmd, "STATUS", 6) == 0) {
        ESP_LOGI(TAG, "Status: mode=%d threshold=%d", current_mode, alarm_threshold);
    } else if (strncmp(cmd, "THRESHOLD:", 10) == 0) {
        int val = atoi(cmd + 10);
        if (val > 0 && val <= 90) {
            alarm_threshold = val;
            ESP_LOGI(TAG, "Threshold: %d", alarm_threshold);
        }
    }
}

// GATT 特征值读回调
static int ble_gap_event(struct ble_gap_event *event, void *arg);

// GATT 特征值写回调
static int gatt_svr_write_cb(uint16_t conn_handle, uint16_t attr_handle,
                              struct ble_gatt_access_ctxt *ctxt, void *arg) {
    if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        process_command(ctxt->om->om_data, ctxt->om->om_len);
    }
    return 0;
}

// GATT 服务定义
static const struct ble_gatt_svc_def gatt_svr_defs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &ble_svc_uuid.u16,
        .characteristics = (struct ble_gatt_chr_def[]){
            {
                .uuid = &ble_gatt_chr_uuid.u16,
                .access_cb = gatt_svr_write_cb,
                .val_handle = NULL,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE_NR,
            },
            {0},
        },
    },
    {0},
};

// GAP 事件处理
static int ble_gap_event(struct ble_gap_event *event, void *arg) {
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            if (event->connect.status == 0) {
                conn_handle = event->connect.conn_handle;
                ESP_LOGI(TAG, "Connected");
            }
            break;
        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI(TAG, "Disconnected");
            conn_handle = 0;
            // 重新开始广播
            ble_gap_adv_start(0, NULL, BLE_HS_FOREVER, NULL, &ble_gap_adv_params);
            break;
        default:
            break;
    }
    return 0;
}

// GAP 初始化
static void ble_app_advertise(void) {
    struct ble_gap_adv_params adv_params = {
        .conn_mode = BLE_GAP_CONN_MODE_UND,
        .itvl_min = 0x20,
        .itvl_max = 0x40,
    };
    ble_gap_adv_start(0, NULL, BLE_HS_FOREVER, &ble_gap_event, &adv_params);
    ESP_LOGI(TAG, "Advertising started");
}

// BLE 主机同步回调
static void ble_app_on_sync(void) {
    ble_hs_util_ensure_addr(0);
    ble_hs_id_infer_auto(0, NULL);
    ble_app_advertise();
}

// BLE 主机任务
static void ble_host_task(void *param) {
    nimble_port_run();  // 此函数不会返回
    vTaskDelete(NULL);
}

esp_err_t ble_control_init(void) {
    ESP_LOGI(TAG, "Initializing NimBLE...");

    // 释放经典蓝牙内存
    esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);

    // 初始化 NimBLE
    ESP_ERROR_CHECK(nimble_port_init());

    // 设置回调
    ble_hs_cfg.reset_cb = NULL;
    ble_hs_cfg.sync_cb = ble_app_on_sync;

    // 注册 GATT 服务
    ble_gatts_count_cfg(gatt_svr_defs);
    ble_gatts_add_svcs(gatt_svr_defs);

    // 设置设备名
    ble_svc_gap_init();
    ble_svc_gap_device_name_set(DEVICE_NAME);

    // 启动 NimBLE 任务
    nimble_port_freertos_init(ble_host_task);

    ESP_LOGI(TAG, "BLE initialized");
    return ESP_OK;
}

ble_mode_t ble_control_get_mode(void) {
    return current_mode;
}

int ble_control_get_threshold(void) {
    return alarm_threshold;
}

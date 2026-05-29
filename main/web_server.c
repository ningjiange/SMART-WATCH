// main/web_server.c — WiFi AP + HTTP Server
#include "web_server.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include <string.h>
#include <sys/param.h>

static const char *TAG = "WEB_SERVER";

// WiFi AP 配置
#define WIFI_SSID       "IMU-Visualizer"
#define WIFI_PASS       "12345678"
#define WIFI_CHANNEL    1
#define MAX_STA_CONN    4

// 全局传感器数据
static mpu6050_data_t g_data = {0};
static int g_mode = 0;  // 0=FREE, 1=LOCK

// ===== HTML 页面 =====
static const char html_page[] = "<!DOCTYPE html>"
"<html><head>"
"<meta charset='UTF-8'>"
"<meta name='viewport' content='width=device-width,initial-scale=1'>"
"<title>IMU Gesture Visualizer</title>"
"<style>"
"body{font-family:Arial;margin:0;padding:20px;background:#1a1a2e;color:#fff}"
"h1{text-align:center;color:#0f0}"
".container{max-width:600px;margin:0 auto}"
".card{background:#16213e;border-radius:10px;padding:20px;margin:10px 0}"
".value{font-size:2em;text-align:center}"
".label{color:#888;text-align:center}"
".green{color:#0f0}.blue{color:#0af}"
"canvas{width:100%;height:200px;background:#0a0a1a;border-radius:5px}"
".btn{display:block;width:100%;padding:15px;margin:10px 0;border:none;border-radius:5px;font-size:1.2em;cursor:pointer}"
".btn-free{background:#0f0;color:#000}"
".btn-lock{background:#f00;color:#fff}"
"</style>"
"</head><body>"
"<div class='container'>"
"<h1>IMU Gesture Visualizer</h1>"
"<div class='card'>"
"<div class='label'>Pitch</div>"
"<div class='value green' id='pitch'>0.0</div>"
"</div>"
"<div class='card'>"
"<div class='label'>Roll</div>"
"<div class='value blue' id='roll'>0.0</div>"
"</div>"
"<div class='card'>"
"<canvas id='chart'></canvas>"
"</div>"
"<button class='btn btn-free' id='modeBtn' onclick='toggleMode()'>Mode: FREE</button>"
"</div>"
"<script>"
"var mode=0,histP=[],histR=[],maxH=60;"
"var ctx=document.getElementById('chart').getContext('2d');"
"function fetchData(){"
"fetch('/api/data').then(r=>r.json()).then(d=>{"
"document.getElementById('pitch').textContent=d.pitch.toFixed(1);"
"document.getElementById('roll').textContent=d.roll.toFixed(1);"
"histP.push(d.pitch);histR.push(d.roll);"
"if(histP.length>maxH){histP.shift();histR.shift();}"
"drawChart();"
"});}"
"function drawChart(){"
"var w=ctx.canvas.width,h=ctx.canvas.height;"
"ctx.clearRect(0,0,w,h);"
"ctx.strokeStyle='#0f0';ctx.lineWidth=2;ctx.beginPath();"
"for(var i=0;i<histP.length;i++){"
"var x=i/(maxH-1)*w;"
"var y=h/2-histP[i]/90*(h/2);"
"i===0?ctx.moveTo(x,y):ctx.lineTo(x,y);"
"}ctx.stroke();"
"ctx.strokeStyle='#0af';ctx.beginPath();"
"for(var i=0;i<histR.length;i++){"
"var x=i/(maxH-1)*w;"
"var y=h/2-histR[i]/90*(h/2);"
"i===0?ctx.moveTo(x,y):ctx.lineTo(x,y);"
"}ctx.stroke();"
"ctx.strokeStyle='#333';ctx.beginPath();"
"ctx.moveTo(0,h/2);ctx.lineTo(w,h/2);ctx.stroke();"
"}"
"function toggleMode(){"
"mode=mode===0?1:0;"
"fetch('/api/mode?set='+mode);"
"var btn=document.getElementById('modeBtn');"
"btn.textContent='Mode: '+(mode===0?'FREE':'LOCK');"
"btn.className='btn '+(mode===0?'btn-free':'btn-lock');"
"}"
"setInterval(fetchData,200);"
"fetchData();"
"</script>"
"</body></html>";

// ===== HTTP Handler =====

// 首页
static esp_err_t index_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, html_page, sizeof(html_page) - 1);
}

// API: 获取数据
static esp_err_t api_data_handler(httpd_req_t *req) {
    char buf[128];
    snprintf(buf, sizeof(buf),
             "{\"pitch\":%.2f,\"roll\":%.2f,\"mode\":%d}",
             g_data.pitch, g_data.roll, g_mode);
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, buf, strlen(buf));
}

// API: 设置模式
static esp_err_t api_mode_handler(httpd_req_t *req) {
    char buf[16];
    if (httpd_req_get_url_query_str(req, buf, sizeof(buf)) == ESP_OK) {
        char val[8];
        if (httpd_query_key_value(buf, "set", val, sizeof(val)) == ESP_OK) {
            g_mode = atoi(val);
        }
    }
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, "{\"ok\":true}", 11);
}

// ===== WiFi =====

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "Station connected, AID=%d", event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "Station disconnected, AID=%d", event->aid);
    }
}

static void wifi_init_softap(void) {
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                    &wifi_event_handler, NULL, NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = WIFI_SSID,
            .ssid_len = strlen(WIFI_SSID),
            .channel = WIFI_CHANNEL,
            .password = WIFI_PASS,
            .max_connection = MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
        },
    };

    if (strlen(WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi AP started: SSID=%s, PASS=%s", WIFI_SSID, WIFI_PASS);
}

// ===== HTTP Server =====

static httpd_handle_t start_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 8;

    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server");
        return NULL;
    }

    // 注册 URI
    httpd_uri_t uri_index = {
        .uri = "/", .method = HTTP_GET,
        .handler = index_handler
    };
    httpd_register_uri_handler(server, &uri_index);

    httpd_uri_t uri_data = {
        .uri = "/api/data", .method = HTTP_GET,
        .handler = api_data_handler
    };
    httpd_register_uri_handler(server, &uri_data);

    httpd_uri_t uri_mode = {
        .uri = "/api/mode", .method = HTTP_GET,
        .handler = api_mode_handler
    };
    httpd_register_uri_handler(server, &uri_mode);

    ESP_LOGI(TAG, "HTTP server started");
    return server;
}

// ===== 公共接口 =====

esp_err_t web_server_init(void) {
    ESP_LOGI(TAG, "Initializing WiFi AP...");

    // 初始化 NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_softap();

    start_webserver();

    return ESP_OK;
}

void web_server_update_data(const mpu6050_data_t *data) {
    g_data = *data;
}

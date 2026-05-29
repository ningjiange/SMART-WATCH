// main/display.c — LVGL 8.4 + ILI9341 显示模块（竖屏 240x320）
#include "display.h"
#include "esp_log.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"
#include <string.h>

static const char *TAG = "DISPLAY";

#define LCD_HOST        SPI2_HOST
#define LCD_SCLK_PIN    18
#define LCD_MOSI_PIN    23
#define LCD_DC_PIN      2
#define LCD_CS_PIN      15
#define LCD_RST_PIN     4

#define LCD_H_RES       240
#define LCD_V_RES       320

static spi_device_handle_t spi;

// ===== LVGL 颜色主题 =====
#define COLOR_BG        lv_color_make(10, 10, 30)     // 深蓝黑背景
#define COLOR_PANEL     lv_color_make(20, 30, 60)     // 卡片背景
#define COLOR_TIME      lv_color_make(0, 255, 180)    // 时间-青绿
#define COLOR_TEMP      lv_color_make(255, 100, 50)   // 温度-橙红
#define COLOR_HUMI      lv_color_make(50, 150, 255)   // 湿度-蓝
#define COLOR_PITCH     lv_color_make(0, 255, 0)      // Pitch-绿
#define COLOR_ROLL      lv_color_make(0, 128, 255)    // Roll-蓝
#define COLOR_WEATHER   lv_color_make(200, 200, 200)  // 天气-浅灰
#define COLOR_BAR_BG    lv_color_make(40, 40, 60)     // 进度条背景

// ===== UI 组件句柄 =====
static lv_obj_t *label_title = NULL;
static lv_obj_t *label_time = NULL;
static lv_obj_t *label_weather = NULL;
static lv_obj_t *label_temp = NULL;
static lv_obj_t *label_humi = NULL;
static lv_obj_t *label_pitch = NULL;
static lv_obj_t *bar_pitch = NULL;
static lv_obj_t *label_roll = NULL;
static lv_obj_t *bar_roll = NULL;

// ===== SPI 底层 =====
static void spi_write(uint8_t *data, size_t len) {
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = data,
    };
    spi_device_polling_transmit(spi, &t);
}

static void lcd_cmd(uint8_t cmd) {
    gpio_set_level(LCD_DC_PIN, 0);
    spi_write(&cmd, 1);
}

static void lcd_data(uint8_t val) {
    gpio_set_level(LCD_DC_PIN, 1);
    spi_write(&val, 1);
}

static void lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    lcd_cmd(0x2A);
    gpio_set_level(LCD_DC_PIN, 1);
    uint8_t buf[4] = {x0 >> 8, x0 & 0xFF, x1 >> 8, x1 & 0xFF};
    spi_write(buf, 4);

    lcd_cmd(0x2B);
    gpio_set_level(LCD_DC_PIN, 1);
    uint8_t buf2[4] = {y0 >> 8, y0 & 0xFF, y1 >> 8, y1 & 0xFF};
    spi_write(buf2, 4);

    lcd_cmd(0x2C);
}

// LVGL 8.x 刷新回调
static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;
    uint32_t size = w * h;

    lcd_set_window(area->x1, area->y1, area->x2, area->y2);

    gpio_set_level(LCD_DC_PIN, 1);
    uint16_t *pixels = (uint16_t *)color_p;
    for (uint32_t i = 0; i < size; i++) {
        uint8_t buf[2];
        buf[0] = pixels[i] >> 8;
        buf[1] = pixels[i] & 0xFF;
        spi_write(buf, 2);
    }

    lv_disp_flush_ready(drv);
}

// ===== 创建卡片容器 =====
static lv_obj_t *create_card(lv_obj_t *parent, lv_coord_t y, lv_coord_t h) {
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, LCD_H_RES - 16, h);
    lv_obj_align(card, LV_ALIGN_TOP_MID, 0, y);
    lv_obj_set_style_bg_color(card, COLOR_PANEL, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_radius(card, 6, 0);
    lv_obj_set_style_pad_all(card, 4, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    return card;
}

// ===== UI 布局 =====
// 竖屏 240x320 布局：
//   y=2..25    标题 "Smart Watch"
//   y=30..65   温度卡片
//   y=70..105  湿度卡片
//   y=110..145 Pitch 进度条
//   y=150..185 Roll 进度条
//   y=190..215 天气文本
//   y=280..320 时间显示（底部）

static void create_ui(void) {
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, COLOR_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    // --- 标题（y=2） ---
    label_title = lv_label_create(scr);
    lv_label_set_text(label_title, "Smart Watch");
    lv_obj_set_style_text_color(label_title, COLOR_WEATHER, 0);
    lv_obj_set_style_text_font(label_title, &lv_font_montserrat_16, 0);
    lv_obj_align(label_title, LV_ALIGN_TOP_MID, 0, 2);

    // --- 温度卡片（y=28） ---
    lv_obj_t *card_temp = create_card(scr, 28, 35);
    label_temp = lv_label_create(card_temp);
    lv_label_set_text(label_temp, "Temp: 25.0" "\xc2\xb0" "C");
    lv_obj_set_style_text_color(label_temp, COLOR_TEMP, 0);
    lv_obj_set_style_text_font(label_temp, &lv_font_montserrat_14, 0);
    lv_obj_align(label_temp, LV_ALIGN_LEFT_MID, 4, 0);

    // --- 湿度卡片（y=68） ---
    lv_obj_t *card_humi = create_card(scr, 68, 35);
    label_humi = lv_label_create(card_humi);
    lv_label_set_text(label_humi, "Humi: 50.0%");
    lv_obj_set_style_text_color(label_humi, COLOR_HUMI, 0);
    lv_obj_set_style_text_font(label_humi, &lv_font_montserrat_14, 0);
    lv_obj_align(label_humi, LV_ALIGN_LEFT_MID, 4, 0);

    // --- Pitch 区域（y=108） ---
    label_pitch = lv_label_create(scr);
    lv_label_set_text(label_pitch, "Pitch: 0.0" "\xc2\xb0");
    lv_obj_set_style_text_color(label_pitch, COLOR_PITCH, 0);
    lv_obj_set_style_text_font(label_pitch, &lv_font_montserrat_12, 0);
    lv_obj_align(label_pitch, LV_ALIGN_TOP_LEFT, 12, 108);

    bar_pitch = lv_bar_create(scr);
    lv_obj_set_size(bar_pitch, LCD_H_RES - 24, 12);
    lv_bar_set_range(bar_pitch, 0, 100);
    lv_bar_set_value(bar_pitch, 50, LV_ANIM_OFF);
    lv_obj_align(bar_pitch, LV_ALIGN_TOP_LEFT, 12, 126);
    lv_obj_set_style_bg_color(bar_pitch, COLOR_BAR_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar_pitch, COLOR_PITCH, LV_PART_INDICATOR);

    // --- Roll 区域（y=148） ---
    label_roll = lv_label_create(scr);
    lv_label_set_text(label_roll, "Roll: 0.0" "\xc2\xb0");
    lv_obj_set_style_text_color(label_roll, COLOR_ROLL, 0);
    lv_obj_set_style_text_font(label_roll, &lv_font_montserrat_12, 0);
    lv_obj_align(label_roll, LV_ALIGN_TOP_LEFT, 12, 148);

    bar_roll = lv_bar_create(scr);
    lv_obj_set_size(bar_roll, LCD_H_RES - 24, 12);
    lv_bar_set_range(bar_roll, 0, 100);
    lv_bar_set_value(bar_roll, 50, LV_ANIM_OFF);
    lv_obj_align(bar_roll, LV_ALIGN_TOP_LEFT, 12, 166);
    lv_obj_set_style_bg_color(bar_roll, COLOR_BAR_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar_roll, COLOR_ROLL, LV_PART_INDICATOR);

    // --- 天气文本（y=188） ---
    label_weather = lv_label_create(scr);
    lv_label_set_text(label_weather, "Weather: --");
    lv_obj_set_style_text_color(label_weather, COLOR_WEATHER, 0);
    lv_obj_set_style_text_font(label_weather, &lv_font_montserrat_12, 0);
    lv_obj_align(label_weather, LV_ALIGN_TOP_MID, 0, 188);

    // --- 时间显示（底部 y=285） ---
    label_time = lv_label_create(scr);
    lv_label_set_text(label_time, "12:34");
    lv_obj_set_style_text_color(label_time, COLOR_TIME, 0);
    lv_obj_set_style_text_font(label_time, &lv_font_montserrat_28, 0);
    lv_obj_align(label_time, LV_ALIGN_BOTTOM_MID, 0, -15);
}

// ===== 初始化 =====
esp_err_t display_init(void) {
    ESP_LOGI(TAG, "Initializing display...");

    // 1. SPI
    spi_bus_config_t bus_cfg = {
        .sclk_io_num = LCD_SCLK_PIN,
        .mosi_io_num = LCD_MOSI_PIN,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &bus_cfg, SPI_DMA_CH_AUTO));

    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = 10 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = LCD_CS_PIN,
        .queue_size = 7,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(LCD_HOST, &dev_cfg, &spi));

    // 2. GPIO
    gpio_set_direction(LCD_RST_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(LCD_DC_PIN, GPIO_MODE_OUTPUT);

    // 3. 硬复位
    gpio_set_level(LCD_RST_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(LCD_RST_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(120));

    // 4. ILI9341 初始化
    lcd_cmd(0x01);
    vTaskDelay(pdMS_TO_TICKS(150));
    lcd_cmd(0x11);
    vTaskDelay(pdMS_TO_TICKS(120));
    lcd_cmd(0x36);
    lcd_data(0x48);  // 竖屏：MV=0, MX=0, MY=1
    lcd_cmd(0x3A);
    lcd_data(0x55);  // 16-bit color
    lcd_cmd(0x29);
    vTaskDelay(pdMS_TO_TICKS(50));

    ESP_LOGI(TAG, "ILI9341 initialized (240x320 portrait)");

    // 5. LVGL 初始化
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

    // 6. LVGL 显示驱动
    static lv_disp_draw_buf_t draw_buf;
    static uint8_t buf1[LCD_H_RES * 40 * sizeof(lv_color_t)];
    lv_disp_draw_buf_init(&draw_buf, buf1, NULL, LCD_H_RES * 40);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = LCD_H_RES;
    disp_drv.ver_res = LCD_V_RES;
    disp_drv.flush_cb = lvgl_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    ESP_LOGI(TAG, "Display initialized");
    return ESP_OK;
}

// ===== 更新显示 =====
void display_update(const sensor_display_data_t *data) {
    if (!label_time) {
        create_ui();
    }

    char buf[48];

    // 更新时间
    lv_label_set_text(label_time, data->time_str);

    // 更新天气
    lv_label_set_text(label_weather, data->weather);

    // 更新温度
    snprintf(buf, sizeof(buf), "Temp: %.1f" "\xc2\xb0" "C", data->temperature);
    lv_label_set_text(label_temp, buf);

    // 更新湿度
    snprintf(buf, sizeof(buf), "Humi: %.1f%%", data->humidity);
    lv_label_set_text(label_humi, buf);

    // 更新 Pitch
    snprintf(buf, sizeof(buf), "Pitch: %+.1f" "\xc2\xb0", data->pitch);
    lv_label_set_text(label_pitch, buf);
    lv_bar_set_value(bar_pitch, (int)((data->pitch + 90.0f) * 100.0f / 180.0f), LV_ANIM_OFF);

    // 更新 Roll
    snprintf(buf, sizeof(buf), "Roll: %+.1f" "\xc2\xb0", data->roll);
    lv_label_set_text(label_roll, buf);
    lv_bar_set_value(bar_roll, (int)((data->roll + 90.0f) * 100.0f / 180.0f), LV_ANIM_OFF);
}

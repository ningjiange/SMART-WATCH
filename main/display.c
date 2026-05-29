// main/display.c — LVGL 8.4 + ILI9341 显示模块
#include "display.h"
#include "cube.h"
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

    // LVGL 8.x 颜色已经是正确的格式，直接发送
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

// UI 组件
static lv_obj_t *label_pitch = NULL;
static lv_obj_t *bar_pitch = NULL;
static lv_obj_t *label_roll = NULL;
static lv_obj_t *bar_roll = NULL;

static void create_ui(void) {
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);

    // 标题
    lv_obj_t *label_title = lv_label_create(scr);
    lv_label_set_text(label_title, "IMU Gesture Visualizer");
    lv_obj_set_style_text_color(label_title, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_title, &lv_font_montserrat_14, 0);
    lv_obj_align(label_title, LV_ALIGN_TOP_MID, 0, 5);

    // 3D 立方体（上半部分）
    cube_init(scr);

    // Pitch 标签（下半部分）
    label_pitch = lv_label_create(scr);
    lv_label_set_text(label_pitch, "Pitch: 0.0");
    lv_obj_set_style_text_color(label_pitch, lv_color_make(0, 255, 0), 0);
    lv_obj_set_style_text_font(label_pitch, &lv_font_montserrat_14, 0);
    lv_obj_align(label_pitch, LV_ALIGN_LEFT_MID, 20, 40);

    // Pitch 进度条
    bar_pitch = lv_bar_create(scr);
    lv_obj_set_size(bar_pitch, 200, 15);
    lv_bar_set_range(bar_pitch, 0, 100);
    lv_bar_set_value(bar_pitch, 50, LV_ANIM_OFF);
    lv_obj_align(bar_pitch, LV_ALIGN_LEFT_MID, 20, 60);
    lv_obj_set_style_bg_color(bar_pitch, lv_color_make(50, 50, 50), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar_pitch, lv_color_make(0, 255, 0), LV_PART_INDICATOR);

    // Roll 标签
    label_roll = lv_label_create(scr);
    lv_label_set_text(label_roll, "Roll: 0.0");
    lv_obj_set_style_text_color(label_roll, lv_color_make(0, 128, 255), 0);
    lv_obj_set_style_text_font(label_roll, &lv_font_montserrat_14, 0);
    lv_obj_align(label_roll, LV_ALIGN_LEFT_MID, 20, 90);

    // Roll 进度条
    bar_roll = lv_bar_create(scr);
    lv_obj_set_size(bar_roll, 200, 15);
    lv_bar_set_range(bar_roll, 0, 100);
    lv_bar_set_value(bar_roll, 50, LV_ANIM_OFF);
    lv_obj_align(bar_roll, LV_ALIGN_LEFT_MID, 20, 110);
    lv_obj_set_style_bg_color(bar_roll, lv_color_make(50, 50, 50), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar_roll, lv_color_make(0, 128, 255), LV_PART_INDICATOR);

    // 状态栏
    lv_obj_t *label_status = lv_label_create(scr);
    lv_label_set_text(label_status, "Mode: FREE | WiFi: --");
    lv_obj_set_style_text_color(label_status, lv_color_make(128, 128, 128), 0);
    lv_obj_set_style_text_font(label_status, &lv_font_montserrat_14, 0);
    lv_obj_align(label_status, LV_ALIGN_BOTTOM_MID, 0, -5);
}

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
    lcd_data(0x88);
    lcd_cmd(0x3A);
    lcd_data(0x55);
    lcd_cmd(0x29);
    vTaskDelay(pdMS_TO_TICKS(50));

    ESP_LOGI(TAG, "ILI9341 initialized");

    // 5. LVGL 初始化
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

    // 6. LVGL 显示驱动（LVGL 8.x API）
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

void display_update(const mpu6050_data_t *data) {
    if (!label_pitch) {
        create_ui();
    }

    // 更新 3D 立方体
    cube_update(data->pitch, data->roll);

    // 更新角度显示
    char buf[32];
    snprintf(buf, sizeof(buf), "Pitch: %+.1f", data->pitch);
    lv_label_set_text(label_pitch, buf);
    lv_bar_set_value(bar_pitch, (int)((data->pitch + 90.0f) * 100.0f / 180.0f), LV_ANIM_OFF);

    snprintf(buf, sizeof(buf), "Roll: %+.1f", data->roll);
    lv_label_set_text(label_roll, buf);
    lv_bar_set_value(bar_roll, (int)((data->roll + 90.0f) * 100.0f / 180.0f), LV_ANIM_OFF);
}

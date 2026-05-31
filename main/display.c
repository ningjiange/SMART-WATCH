// main/display.c — LVGL 8.4 + ILI9341 显示模块（多页面版本）
#include "display.h"
#include "esp_log.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"
#include <string.h>
#include "page_manager.h"
#include "flashlight.h"
#include "system_info.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "DISPLAY";

// LVGL 操作互斥锁
static SemaphoreHandle_t lvgl_mutex = NULL;

#define LCD_HOST        SPI2_HOST
#define LCD_SCLK_PIN    18
#define LCD_MOSI_PIN    23
#define LCD_DC_PIN      2
#define LCD_CS_PIN      15
#define LCD_RST_PIN     4

#define LCD_H_RES       240
#define LCD_V_RES       320

static spi_device_handle_t spi;

// ===== 底层 SPI 驱动 =====

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

// ===== LVGL 刷新回调 =====

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

// ===== UI 组件（HOME 页面）=====

static lv_obj_t *label_temp = NULL;
static lv_obj_t *label_humi = NULL;
static lv_obj_t *label_pitch = NULL;
static lv_obj_t *bar_pitch = NULL;
static lv_obj_t *label_roll = NULL;
static lv_obj_t *bar_roll = NULL;
static lv_obj_t *label_weather = NULL;
static lv_obj_t *label_time = NULL;

// ===== UI 组件（SPORT 页面）=====

static lv_obj_t *sport_label_title = NULL;
static lv_obj_t *sport_label_steps = NULL;
static lv_obj_t *sport_label_motion = NULL;
static lv_obj_t *sport_label_calories = NULL;

// ===== UI 组件（TOOLS 页面）=====

static lv_obj_t *tools_label_title = NULL;
static lv_obj_t *tools_label_stopwatch = NULL;
static lv_obj_t *tools_label_countdown = NULL;
static lv_obj_t *tools_label_hint = NULL;

// ===== UI 组件（FLASHLIGHT 页面）=====

static lv_obj_t *flashlight_label_status = NULL;

// ===== UI 组件（SYSTEM INFO 页面）=====

static lv_obj_t *sysinfo_label_title = NULL;
static lv_obj_t *sysinfo_label_cpu_temp = NULL;
static lv_obj_t *sysinfo_label_wifi = NULL;
static lv_obj_t *sysinfo_label_memory = NULL;

// ===== UI 组件（通用）=====

static lv_obj_t *page_indicator = NULL;
static lv_obj_t *label_page = NULL;

// 当前显示的页面
static page_id_t current_display_page = PAGE_COUNT;  // 无效值，强制首次创建

// ===== 创建 HOME 页面 =====

static void create_home_page(void) {
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);

    // 标题
    lv_obj_t *label_title = lv_label_create(scr);
    lv_label_set_text(label_title, "Smart Watch");
    lv_obj_set_style_text_color(label_title, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_title, &lv_font_montserrat_14, 0);
    lv_obj_align(label_title, LV_ALIGN_TOP_MID, 0, 5);

    // 温度
    label_temp = lv_label_create(scr);
    lv_label_set_text(label_temp, "Temp: --");
    lv_obj_set_style_text_color(label_temp, lv_color_make(255, 100, 0), 0);
    lv_obj_set_style_text_font(label_temp, &lv_font_montserrat_14, 0);
    lv_obj_align(label_temp, LV_ALIGN_LEFT_MID, 20, -60);

    // 湿度
    label_humi = lv_label_create(scr);
    lv_label_set_text(label_humi, "Humi: --");
    lv_obj_set_style_text_color(label_humi, lv_color_make(0, 128, 255), 0);
    lv_obj_set_style_text_font(label_humi, &lv_font_montserrat_14, 0);
    lv_obj_align(label_humi, LV_ALIGN_LEFT_MID, 20, -40);

    // Pitch
    label_pitch = lv_label_create(scr);
    lv_label_set_text(label_pitch, "Pitch: 0.0");
    lv_obj_set_style_text_color(label_pitch, lv_color_make(0, 255, 0), 0);
    lv_obj_set_style_text_font(label_pitch, &lv_font_montserrat_14, 0);
    lv_obj_align(label_pitch, LV_ALIGN_LEFT_MID, 20, 0);

    // Pitch 进度条
    bar_pitch = lv_bar_create(scr);
    lv_obj_set_size(bar_pitch, 200, 10);
    lv_bar_set_range(bar_pitch, 0, 100);
    lv_bar_set_value(bar_pitch, 50, LV_ANIM_OFF);
    lv_obj_align(bar_pitch, LV_ALIGN_LEFT_MID, 20, 20);
    lv_obj_set_style_bg_color(bar_pitch, lv_color_make(50, 50, 50), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar_pitch, lv_color_make(0, 255, 0), LV_PART_INDICATOR);

    // Roll
    label_roll = lv_label_create(scr);
    lv_label_set_text(label_roll, "Roll: 0.0");
    lv_obj_set_style_text_color(label_roll, lv_color_make(0, 128, 255), 0);
    lv_obj_set_style_text_font(label_roll, &lv_font_montserrat_14, 0);
    lv_obj_align(label_roll, LV_ALIGN_LEFT_MID, 20, 40);

    // Roll 进度条
    bar_roll = lv_bar_create(scr);
    lv_obj_set_size(bar_roll, 200, 10);
    lv_bar_set_range(bar_roll, 0, 100);
    lv_bar_set_value(bar_roll, 50, LV_ANIM_OFF);
    lv_obj_align(bar_roll, LV_ALIGN_LEFT_MID, 20, 60);
    lv_obj_set_style_bg_color(bar_roll, lv_color_make(50, 50, 50), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar_roll, lv_color_make(0, 128, 255), LV_PART_INDICATOR);

    // 天气
    label_weather = lv_label_create(scr);
    lv_label_set_text(label_weather, "Weather: --");
    lv_obj_set_style_text_color(label_weather, lv_color_make(255, 255, 0), 0);
    lv_obj_set_style_text_font(label_weather, &lv_font_montserrat_14, 0);
    lv_obj_align(label_weather, LV_ALIGN_LEFT_MID, 20, 90);

    // 时间
    label_time = lv_label_create(scr);
    lv_label_set_text(label_time, "--:--");
    lv_obj_set_style_text_color(label_time, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_time, &lv_font_montserrat_28, 0);
    lv_obj_align(label_time, LV_ALIGN_BOTTOM_MID, 0, -10);

    // 页面指示器
    page_indicator = lv_label_create(scr);
    lv_label_set_text(page_indicator, "HOME");
    lv_obj_set_style_text_color(page_indicator, lv_color_make(128, 128, 128), 0);
    lv_obj_set_style_text_font(page_indicator, &lv_font_montserrat_12, 0);
    lv_obj_align(page_indicator, LV_ALIGN_TOP_RIGHT, -10, 5);
}

// ===== 创建 SPORT 页面 =====

static void create_sport_page(void) {
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);

    // 标题
    sport_label_title = lv_label_create(scr);
    lv_label_set_text(sport_label_title, "Sport Mode");
    lv_obj_set_style_text_color(sport_label_title, lv_color_make(0, 255, 128), 0);
    lv_obj_set_style_text_font(sport_label_title, &lv_font_montserrat_14, 0);
    lv_obj_align(sport_label_title, LV_ALIGN_TOP_MID, 0, 20);

    // 步数
    sport_label_steps = lv_label_create(scr);
    lv_label_set_text(sport_label_steps, "Steps: 0");
    lv_obj_set_style_text_color(sport_label_steps, lv_color_white(), 0);
    lv_obj_set_style_text_font(sport_label_steps, &lv_font_montserrat_24, 0);
    lv_obj_align(sport_label_steps, LV_ALIGN_CENTER, 0, -30);

    // 运动状态
    sport_label_motion = lv_label_create(scr);
    lv_label_set_text(sport_label_motion, "Status: Static");
    lv_obj_set_style_text_color(sport_label_motion, lv_color_make(255, 255, 0), 0);
    lv_obj_set_style_text_font(sport_label_motion, &lv_font_montserrat_14, 0);
    lv_obj_align(sport_label_motion, LV_ALIGN_CENTER, 0, 10);

    // 卡路里
    sport_label_calories = lv_label_create(scr);
    lv_label_set_text(sport_label_calories, "Calories: 0 kcal");
    lv_obj_set_style_text_color(sport_label_calories, lv_color_make(255, 128, 0), 0);
    lv_obj_set_style_text_font(sport_label_calories, &lv_font_montserrat_14, 0);
    lv_obj_align(sport_label_calories, LV_ALIGN_CENTER, 0, 50);

    // 页面指示器
    page_indicator = lv_label_create(scr);
    lv_label_set_text(page_indicator, "SPORT");
    lv_obj_set_style_text_color(page_indicator, lv_color_make(128, 128, 128), 0);
    lv_obj_set_style_text_font(page_indicator, &lv_font_montserrat_12, 0);
    lv_obj_align(page_indicator, LV_ALIGN_TOP_RIGHT, -10, 5);
}

// ===== 创建 TOOLS 页面 =====

static void create_tools_page(void) {
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);

    // 标题
    tools_label_title = lv_label_create(scr);
    lv_label_set_text(tools_label_title, "Tools");
    lv_obj_set_style_text_color(tools_label_title, lv_color_make(128, 128, 255), 0);
    lv_obj_set_style_text_font(tools_label_title, &lv_font_montserrat_14, 0);
    lv_obj_align(tools_label_title, LV_ALIGN_TOP_MID, 0, 20);

    // 秒表
    tools_label_stopwatch = lv_label_create(scr);
    lv_label_set_text(tools_label_stopwatch, "> Stopwatch: 00:00.000");
    lv_obj_set_style_text_color(tools_label_stopwatch, lv_color_make(0, 255, 128), 0);
    lv_obj_set_style_text_font(tools_label_stopwatch, &lv_font_montserrat_14, 0);
    lv_obj_align(tools_label_stopwatch, LV_ALIGN_CENTER, 0, -30);

    // 倒计时
    tools_label_countdown = lv_label_create(scr);
    lv_label_set_text(tools_label_countdown, "  Countdown: 01:00.000");
    lv_obj_set_style_text_color(tools_label_countdown, lv_color_make(255, 128, 0), 0);
    lv_obj_set_style_text_font(tools_label_countdown, &lv_font_montserrat_14, 0);
    lv_obj_align(tools_label_countdown, LV_ALIGN_CENTER, 0, 10);

    // 提示文本
    tools_label_hint = lv_label_create(scr);
    lv_label_set_text(tools_label_hint, "SELECT: Start/Stop\nSwitch tool: UP/DOWN");
    lv_obj_set_style_text_color(tools_label_hint, lv_color_make(128, 128, 128), 0);
    lv_obj_set_style_text_font(tools_label_hint, &lv_font_montserrat_12, 0);
    lv_obj_align(tools_label_hint, LV_ALIGN_CENTER, 0, 60);

    // 页面指示器
    page_indicator = lv_label_create(scr);
    lv_label_set_text(page_indicator, "TOOLS");
    lv_obj_set_style_text_color(page_indicator, lv_color_make(128, 128, 128), 0);
    lv_obj_set_style_text_font(page_indicator, &lv_font_montserrat_12, 0);
    lv_obj_align(page_indicator, LV_ALIGN_TOP_RIGHT, -10, 5);
}

// ===== 创建手电筒页面 =====

static void create_flashlight_page(void) {
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);

    // 标题
    lv_obj_t *label_title = lv_label_create(scr);
    lv_label_set_text(label_title, "Flashlight");
    lv_obj_set_style_text_color(label_title, lv_color_make(255, 255, 0), 0);
    lv_obj_set_style_text_font(label_title, &lv_font_montserrat_14, 0);
    lv_obj_align(label_title, LV_ALIGN_TOP_MID, 0, 20);

    // 状态显示
    flashlight_label_status = lv_label_create(scr);
    if (flashlight_is_on()) {
        lv_label_set_text(flashlight_label_status, "ON");
        lv_obj_set_style_text_color(flashlight_label_status, lv_color_make(0, 255, 0), 0);
    } else {
        lv_label_set_text(flashlight_label_status, "OFF");
        lv_obj_set_style_text_color(flashlight_label_status, lv_color_make(128, 128, 128), 0);
    }
    lv_obj_set_style_text_font(flashlight_label_status, &lv_font_montserrat_24, 0);
    lv_obj_align(flashlight_label_status, LV_ALIGN_CENTER, 0, -20);

    // 提示文本
    lv_obj_t *label_hint = lv_label_create(scr);
    lv_label_set_text(label_hint, "SELECT: Toggle ON/OFF");
    lv_obj_set_style_text_color(label_hint, lv_color_make(128, 128, 128), 0);
    lv_obj_set_style_text_font(label_hint, &lv_font_montserrat_12, 0);
    lv_obj_align(label_hint, LV_ALIGN_CENTER, 0, 30);

    // 页面指示器
    page_indicator = lv_label_create(scr);
    lv_label_set_text(page_indicator, "LIGHT");
    lv_obj_set_style_text_color(page_indicator, lv_color_make(128, 128, 128), 0);
    lv_obj_set_style_text_font(page_indicator, &lv_font_montserrat_12, 0);
    lv_obj_align(page_indicator, LV_ALIGN_TOP_RIGHT, -10, 5);
}

// ===== 创建系统信息页面 =====

static void create_sysinfo_page(void) {
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);

    // 标题
    sysinfo_label_title = lv_label_create(scr);
    lv_label_set_text(sysinfo_label_title, "System Info");
    lv_obj_set_style_text_color(sysinfo_label_title, lv_color_make(128, 128, 255), 0);
    lv_obj_set_style_text_font(sysinfo_label_title, &lv_font_montserrat_14, 0);
    lv_obj_align(sysinfo_label_title, LV_ALIGN_TOP_MID, 0, 10);

    // CPU 温度
    sysinfo_label_cpu_temp = lv_label_create(scr);
    lv_label_set_text(sysinfo_label_cpu_temp, "CPU: --°C");
    lv_obj_set_style_text_color(sysinfo_label_cpu_temp, lv_color_make(255, 100, 0), 0);
    lv_obj_set_style_text_font(sysinfo_label_cpu_temp, &lv_font_montserrat_14, 0);
    lv_obj_align(sysinfo_label_cpu_temp, LV_ALIGN_LEFT_MID, 20, -40);

    // WiFi 信号
    sysinfo_label_wifi = lv_label_create(scr);
    lv_label_set_text(sysinfo_label_wifi, "WiFi: -- dBm");
    lv_obj_set_style_text_color(sysinfo_label_wifi, lv_color_make(0, 128, 255), 0);
    lv_obj_set_style_text_font(sysinfo_label_wifi, &lv_font_montserrat_14, 0);
    lv_obj_align(sysinfo_label_wifi, LV_ALIGN_LEFT_MID, 20, 0);

    // 内存使用
    sysinfo_label_memory = lv_label_create(scr);
    lv_label_set_text(sysinfo_label_memory, "RAM: --%");
    lv_obj_set_style_text_color(sysinfo_label_memory, lv_color_make(0, 255, 128), 0);
    lv_obj_set_style_text_font(sysinfo_label_memory, &lv_font_montserrat_14, 0);
    lv_obj_align(sysinfo_label_memory, LV_ALIGN_LEFT_MID, 20, 40);

    // 页面指示器
    page_indicator = lv_label_create(scr);
    lv_label_set_text(page_indicator, "SYS");
    lv_obj_set_style_text_color(page_indicator, lv_color_make(128, 128, 128), 0);
    lv_obj_set_style_text_font(page_indicator, &lv_font_montserrat_12, 0);
    lv_obj_align(page_indicator, LV_ALIGN_TOP_RIGHT, -10, 10);
}

// ===== 创建占位页面 =====

static void create_placeholder_page(const char *title) {
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);

    // 标题
    lv_obj_t *label_title = lv_label_create(scr);
    lv_label_set_text(label_title, title);
    lv_obj_set_style_text_color(label_title, lv_color_make(128, 128, 128), 0);
    lv_obj_set_style_text_font(label_title, &lv_font_montserrat_14, 0);
    lv_obj_align(label_title, LV_ALIGN_TOP_MID, 0, 20);

    // 占位文本
    lv_obj_t *label_placeholder = lv_label_create(scr);
    lv_label_set_text(label_placeholder, "Coming Soon...");
    lv_obj_set_style_text_color(label_placeholder, lv_color_make(128, 128, 128), 0);
    lv_obj_set_style_text_font(label_placeholder, &lv_font_montserrat_12, 0);
    lv_obj_align(label_placeholder, LV_ALIGN_CENTER, 0, 0);

    // 页面指示器
    page_indicator = lv_label_create(scr);
    lv_label_set_text(page_indicator, title);
    lv_obj_set_style_text_color(page_indicator, lv_color_make(128, 128, 128), 0);
    lv_obj_set_style_text_font(page_indicator, &lv_font_montserrat_12, 0);
    lv_obj_align(page_indicator, LV_ALIGN_TOP_RIGHT, -10, 5);
}

// ===== 清除当前页面 =====

static void clear_current_page(void) {
    lv_obj_t *scr = lv_scr_act();

    // 逐个删除所有子对象
    uint32_t child_count = lv_obj_get_child_cnt(scr);
    for (int32_t i = child_count - 1; i >= 0; i--) {
        lv_obj_t *child = lv_obj_get_child(scr, i);
        if (child) {
            lv_obj_del(child);
        }
    }

    // 等待 LVGL 处理删除操作
    vTaskDelay(pdMS_TO_TICKS(10));

    // 强制刷新显示
    lv_refr_now(NULL);

    // 清空所有指针
    label_temp = NULL;
    label_humi = NULL;
    label_pitch = NULL;
    bar_pitch = NULL;
    label_roll = NULL;
    bar_roll = NULL;
    label_weather = NULL;
    label_time = NULL;

    sport_label_title = NULL;
    sport_label_steps = NULL;
    sport_label_motion = NULL;
    sport_label_calories = NULL;

    tools_label_title = NULL;
    tools_label_stopwatch = NULL;
    tools_label_countdown = NULL;
    tools_label_hint = NULL;

    flashlight_label_status = NULL;

    sysinfo_label_title = NULL;
    sysinfo_label_cpu_temp = NULL;
    sysinfo_label_wifi = NULL;
    sysinfo_label_memory = NULL;

    page_indicator = NULL;
}

// ===== 切换页面 =====

static void switch_page(page_id_t new_page) {
    if (new_page == current_display_page) {
        return;
    }

    // 获取互斥锁，确保页面切换原子性
    if (xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(50)) != pdTRUE) {
        return;
    }

    ESP_LOGI(TAG, "Switching page from %d to %d", current_display_page, new_page);

    // 清除当前页面
    clear_current_page();

    // 创建新页面
    switch (new_page) {
        case PAGE_HOME:
            create_home_page();
            break;
        case PAGE_SPORT:
            create_sport_page();
            break;
        case PAGE_TOOLS:
            create_tools_page();
            break;
        case PAGE_GAME:
            create_flashlight_page();
            break;
        case PAGE_SETTINGS:
            create_sysinfo_page();
            break;
        default:
            create_home_page();
            break;
    }

    current_display_page = new_page;

    xSemaphoreGive(lvgl_mutex);
}

// ===== 公共接口 =====

esp_err_t display_init(void) {
    ESP_LOGI(TAG, "Initializing display...");

    // 创建 LVGL 互斥锁
    if (lvgl_mutex == NULL) {
        lvgl_mutex = xSemaphoreCreateMutex();
    }

    // 1. SPI 初始化
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
    lcd_data(0x88);  // 竖屏：MY=1, MX=0, BGR
    lcd_cmd(0x3A);
    lcd_data(0x55);
    lcd_cmd(0x29);
    vTaskDelay(pdMS_TO_TICKS(50));

    ESP_LOGI(TAG, "ILI9341 initialized (240x320 portrait)");

    // 5. LVGL 初始化（使用 esp_lvgl_port）
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

    // 6. LVGL 显示驱动
    static lv_disp_draw_buf_t draw_buf;
    static lv_color_t buf1[LCD_H_RES * 40];
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

void display_update(const sensor_display_data_t *data) {
    // 检查页面是否需要切换
    page_id_t target_page = page_manager_get_current();
    switch_page(target_page);

    // 获取互斥锁，确保显示更新原子性
    if (xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(50)) != pdTRUE) {
        return;
    }

    char buf[64];

    // 根据当前页面更新显示
    switch (current_display_page) {
        case PAGE_HOME:
            if (label_time) {
                lv_label_set_text(label_time, data->time_str);
            }
            if (label_weather) {
                lv_label_set_text(label_weather, data->weather);
            }
            if (label_temp) {
                snprintf(buf, sizeof(buf), "Temp: %.1f°C", data->temperature);
                lv_label_set_text(label_temp, buf);
            }
            if (label_humi) {
                snprintf(buf, sizeof(buf), "Humi: %.0f%%", data->humidity);
                lv_label_set_text(label_humi, buf);
            }
            if (label_pitch) {
                snprintf(buf, sizeof(buf), "Pitch: %+.1f", data->pitch);
                lv_label_set_text(label_pitch, buf);
            }
            if (bar_pitch) {
                lv_bar_set_value(bar_pitch, (int)((data->pitch + 90.0f) * 100.0f / 180.0f), LV_ANIM_OFF);
            }
            if (label_roll) {
                snprintf(buf, sizeof(buf), "Roll: %+.1f", data->roll);
                lv_label_set_text(label_roll, buf);
            }
            if (bar_roll) {
                lv_bar_set_value(bar_roll, (int)((data->roll + 90.0f) * 100.0f / 180.0f), LV_ANIM_OFF);
            }
            break;

        case PAGE_SPORT:
            if (sport_label_steps) {
                snprintf(buf, sizeof(buf), "Steps: %lu", data->steps);
                lv_label_set_text(sport_label_steps, buf);
            }
            if (sport_label_motion) {
                const char *motion_str = "Static";
                switch (data->motion_state) {
                    case 1: motion_str = "Walking"; break;
                    case 2: motion_str = "Running"; break;
                }
                snprintf(buf, sizeof(buf), "Status: %s", motion_str);
                lv_label_set_text(sport_label_motion, buf);
            }
            if (sport_label_calories) {
                snprintf(buf, sizeof(buf), "Calories: %.1f kcal", data->calories);
                lv_label_set_text(sport_label_calories, buf);
            }
            break;

        case PAGE_TOOLS:
            if (tools_label_stopwatch) {
                if (data->tool_mode == 0) {
                    // 选中秒表
                    snprintf(buf, sizeof(buf), "> Stopwatch: %s", data->stopwatch_str);
                    lv_obj_set_style_text_color(tools_label_stopwatch, lv_color_make(0, 255, 128), 0);
                } else {
                    // 未选中秒表
                    snprintf(buf, sizeof(buf), "  Stopwatch: %s", data->stopwatch_str);
                    lv_obj_set_style_text_color(tools_label_stopwatch, lv_color_make(128, 128, 128), 0);
                }
                lv_label_set_text(tools_label_stopwatch, buf);
            }
            if (tools_label_countdown) {
                if (data->tool_mode == 1) {
                    // 选中倒计时
                    snprintf(buf, sizeof(buf), "> Countdown: %s", data->countdown_str);
                    lv_obj_set_style_text_color(tools_label_countdown, lv_color_make(255, 200, 0), 0);
                } else {
                    // 未选中倒计时
                    snprintf(buf, sizeof(buf), "  Countdown: %s", data->countdown_str);
                    lv_obj_set_style_text_color(tools_label_countdown, lv_color_make(128, 128, 128), 0);
                }
                lv_label_set_text(tools_label_countdown, buf);
            }
            break;

        case PAGE_GAME:
            // 手电筒页面更新
            if (flashlight_label_status) {
                if (data->flashlight_on) {
                    lv_label_set_text(flashlight_label_status, "ON");
                    lv_obj_set_style_text_color(flashlight_label_status, lv_color_make(0, 255, 0), 0);
                } else {
                    lv_label_set_text(flashlight_label_status, "OFF");
                    lv_obj_set_style_text_color(flashlight_label_status, lv_color_make(128, 128, 128), 0);
                }
            }
            break;

        case PAGE_SETTINGS:
            // 系统信息页面更新
            if (sysinfo_label_cpu_temp) {
                float temp = system_info_get_cpu_temp();
                char buf[32];
                snprintf(buf, sizeof(buf), "CPU: %.1f°C", temp);
                lv_label_set_text(sysinfo_label_cpu_temp, buf);
            }
            if (sysinfo_label_wifi) {
                int8_t rssi = system_info_get_wifi_rssi();
                char buf[32];
                snprintf(buf, sizeof(buf), "WiFi: %d dBm (%s)", rssi, system_info_get_wifi_quality(rssi));
                lv_label_set_text(sysinfo_label_wifi, buf);
            }
            if (sysinfo_label_memory) {
                float usage = system_info_get_heap_usage_percent();
                uint32_t free_heap = system_info_get_free_heap();
                char buf[64];
                snprintf(buf, sizeof(buf), "RAM: %.1f%% used (%lu KB free)", usage, free_heap / 1024);
                lv_label_set_text(sysinfo_label_memory, buf);
            }
            break;

        default:
            break;
    }

    xSemaphoreGive(lvgl_mutex);
}

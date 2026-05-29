// 最小 LCD 测试程序 — 简化版
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "LCD_TEST";

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

static void lcd_fill_color(uint8_t hi, uint8_t lo) {
    gpio_set_level(LCD_DC_PIN, 1);
    uint8_t buf[480];  // 240 pixels * 2 bytes
    for (int i = 0; i < 240; i++) {
        buf[i * 2] = hi;
        buf[i * 2 + 1] = lo;
    }
    for (int y = 0; y < 320; y++) {
        spi_write(buf, 480);
    }
}

void lcd_test_run(void) {
    ESP_LOGI(TAG, "=== LCD Test Start ===");

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

    // 4. ILI9341 基础初始化
    lcd_cmd(0x01);  // Software reset
    vTaskDelay(pdMS_TO_TICKS(150));

    lcd_cmd(0x11);  // Sleep out
    vTaskDelay(pdMS_TO_TICKS(120));

    lcd_cmd(0x36);  // MADCTL
    lcd_data(0x48);  // 竖屏，BGR

    lcd_cmd(0x3A);  // Pixel format
    lcd_data(0x55);  // 16bit

    lcd_cmd(0x29);  // Display on
    vTaskDelay(pdMS_TO_TICKS(50));

    ESP_LOGI(TAG, "Init done");

    // 5. 全屏红色 (大端: 0xF8, 0x00)
    ESP_LOGI(TAG, "RED (big endian)");
    lcd_set_window(0, 0, LCD_H_RES - 1, LCD_V_RES - 1);
    lcd_fill_color(0xF8, 0x00);
    vTaskDelay(pdMS_TO_TICKS(2000));

    // 6. 全屏绿色 (大端: 0x07, 0xE0)
    ESP_LOGI(TAG, "GREEN (big endian)");
    lcd_set_window(0, 0, LCD_H_RES - 1, LCD_V_RES - 1);
    lcd_fill_color(0x07, 0xE0);
    vTaskDelay(pdMS_TO_TICKS(2000));

    // 7. 全屏蓝色 (大端: 0x00, 0x1F)
    ESP_LOGI(TAG, "BLUE (big endian)");
    lcd_set_window(0, 0, LCD_H_RES - 1, LCD_V_RES - 1);
    lcd_fill_color(0x00, 0x1F);
    vTaskDelay(pdMS_TO_TICKS(2000));

    // 8. 全屏白色 (大端: 0xFF, 0xFF)
    ESP_LOGI(TAG, "WHITE (big endian)");
    lcd_set_window(0, 0, LCD_H_RES - 1, LCD_V_RES - 1);
    lcd_fill_color(0xFF, 0xFF);
    vTaskDelay(pdMS_TO_TICKS(2000));

    ESP_LOGI(TAG, "=== Test Complete ===");
}

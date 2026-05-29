#include "dht11.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "rom/ets_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "DHT11";
static gpio_num_t dht11_pin = GPIO_NUM_25;

// 上一次有效数据
static float last_temp = 25.0f;
static float last_humi = 50.0f;

// 等待引脚变为指定状态，超时返回 -1
static int wait_for_level(gpio_num_t pin, int level, int timeout_us) {
    int count = 0;
    while (gpio_get_level(pin) != level) {
        ets_delay_us(1);
        count++;
        if (count > timeout_us) return -1;
    }
    return count;
}

esp_err_t dht11_init(gpio_num_t pin) {
    dht11_pin = pin;
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << pin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    ESP_LOGI(TAG, "DHT11 initialized on GPIO %d", pin);
    return ESP_OK;
}

esp_err_t dht11_read(float *temperature, float *humidity) {
    uint8_t data[5] = {0};

    // 1. 发送起始信号
    gpio_set_direction(dht11_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(dht11_pin, 0);
    ets_delay_us(20000);  // 拉低 20ms
    gpio_set_level(dht11_pin, 1);
    ets_delay_us(40);     // 拉高 40us

    // 2. 切换到输入，等待 DHT11 响应
    gpio_set_direction(dht11_pin, GPIO_MODE_INPUT);

    // 等待低电平（DHT11 响应）
    if (wait_for_level(dht11_pin, 0, 100) < 0) {
        ESP_LOGD(TAG, "No response, using last valid data");
        *temperature = last_temp;
        *humidity = last_humi;
        return ESP_OK;
    }
    // 等待高电平
    if (wait_for_level(dht11_pin, 1, 100) < 0) {
        ESP_LOGD(TAG, "Response timeout, using last valid data");
        *temperature = last_temp;
        *humidity = last_humi;
        return ESP_OK;
    }

    // 3. 读取 40 位数据
    for (int i = 0; i < 40; i++) {
        // 等待低电平（每个位的开始）
        if (wait_for_level(dht11_pin, 0, 100) < 0) {
            ESP_LOGD(TAG, "Bit %d timeout, using last valid data", i);
            *temperature = last_temp;
            *humidity = last_humi;
            return ESP_OK;
        }
        // 等待高电平
        if (wait_for_level(dht11_pin, 1, 100) < 0) {
            ESP_LOGD(TAG, "Bit %d high timeout, using last valid data", i);
            *temperature = last_temp;
            *humidity = last_humi;
            return ESP_OK;
        }
        // 测量高电平持续时间
        ets_delay_us(30);
        if (gpio_get_level(dht11_pin) == 1) {
            data[i / 8] |= (1 << (7 - (i % 8)));
        }
    }

    // 4. 校验
    uint8_t checksum = data[0] + data[1] + data[2] + data[3];
    if (checksum != data[4]) {
        ESP_LOGD(TAG, "Checksum error: %02X != %02X", checksum, data[4]);
        *temperature = last_temp;
        *humidity = last_humi;
        return ESP_OK;
    }

    // 5. 解析数据（DHT11 精度：温度 ±2°C，湿度 ±5%）
    float temp = data[2];  // DHT11 温度只有整数部分
    float humi = data[0];  // DHT11 湿度只有整数部分

    // 数据有效性检查
    if (temp < -20 || temp > 60 || humi < 0 || humi > 100) {
        ESP_LOGD(TAG, "Invalid data: Temp=%.1f, Humi=%.1f", temp, humi);
        *temperature = last_temp;
        *humidity = last_humi;
        return ESP_OK;
    }

    // 更新有效数据
    last_temp = temp;
    last_humi = humi;
    *temperature = temp;
    *humidity = humi;

    ESP_LOGI(TAG, "DHT11: Temp=%.1f°C, Humi=%.1f%%", *temperature, *humidity);
    return ESP_OK;
}

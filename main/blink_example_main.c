/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "led_strip.h"

static const char *TAG = "blink";

enum {
    BLINK_GPIO = 48,
    BLINK_PERIOD_MS = 1000,
    LED_BRIGHTNESS = 16,
};

void app_main(void)
{
    led_strip_handle_t led_strip;
    led_strip_config_t strip_config = {
        .strip_gpio_num = BLINK_GPIO,
        .max_leds = 1,
    };
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000,
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    ESP_ERROR_CHECK(led_strip_clear(led_strip));
    ESP_LOGI(TAG, "Blink GPIO: %d; interval: %d ms", BLINK_GPIO, BLINK_PERIOD_MS);

    bool led_on = false;
    while (1) {
        ESP_LOGI(TAG, "Turning the LED %s!", led_on ? "ON" : "OFF");
        if (led_on) {
            ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, 0,
                                               LED_BRIGHTNESS, LED_BRIGHTNESS, LED_BRIGHTNESS));
            ESP_ERROR_CHECK(led_strip_refresh(led_strip));
        } else {
            ESP_ERROR_CHECK(led_strip_clear(led_strip));
        }
        led_on = !led_on;
        vTaskDelay(pdMS_TO_TICKS(BLINK_PERIOD_MS));
    }
}

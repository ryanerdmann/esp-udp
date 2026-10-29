#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_check.h"
#include "led.h"

static const char *TAG = "blink";

esp_err_t app_run()
{
    led_t led;
    ESP_RETURN_ON_ERROR(led_init(&led), TAG, "Failed to initialize LED strip.");
    ESP_RETURN_ON_ERROR(led_set_state(&led, LED_BLINK), TAG, "Failed to set LED state.");

    ESP_LOGI(TAG, "Set LED state to BLINK...");

    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    return ESP_OK;
}

void app_main(void)
{
    ESP_LOGI(TAG, "Running application...");
    ESP_ERROR_CHECK(app_run());
}

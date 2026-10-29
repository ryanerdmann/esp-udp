#include <stdbool.h>
#include "esp_log.h"
#include "esp_check.h"
#include "led.h"

static const char *TAG = "blink";

enum {
    BLINK_GPIO = 48,
    BLINK_PERIOD_MS = 1000,
    LED_BRIGHTNESS = 16,
};

static const uint32_t LED_TASK_STOP = UINT32_MAX;

static void led_show_state(led_t *led, enum led_state state)
{
    if (state == LED_OFF)
    {
        ESP_ERROR_CHECK(led_strip_clear(led->led_handle));
        return;
    }

    bool white = state == LED_ON_WHITE || state == LED_BLINK_WHITE;
    ESP_ERROR_CHECK(led_strip_set_pixel(led->led_handle, 0,
                                        white || state == LED_ON_RED ? LED_BRIGHTNESS : 0,
                                        white || state == LED_ON_GREEN ? LED_BRIGHTNESS : 0,
                                        white || state == LED_ON_BLUE ? LED_BRIGHTNESS : 0));
    ESP_ERROR_CHECK(led_strip_refresh(led->led_handle));
}

static void led_task(void *pvParameters)
{
    led_t *led = (led_t *) pvParameters;

    enum led_state current_state = LED_OFF;
    bool isOn = false;

    for (;;)
    {
        uint32_t notified_value = 0;
        BaseType_t ret = xTaskNotifyWait(0, 0, &notified_value, pdMS_TO_TICKS(BLINK_PERIOD_MS));

        if (ret == pdTRUE)
        {
            if (notified_value == LED_TASK_STOP)
            {
                break;
            }

            ESP_LOGI(TAG, "Notified with value: %u", notified_value);

            enum led_state next_state = (enum led_state)notified_value;

            if (next_state != current_state)
            {
                led_show_state(led, next_state);
                isOn = next_state != LED_OFF;
                current_state = next_state;
            }
        }
        else
        {
            if (current_state == LED_BLINK_WHITE)
            {
                isOn = !isOn;
                led_show_state(led, isOn ? LED_BLINK_WHITE : LED_OFF);
            }
        }

    }

    // No access to led or its driver is allowed after acknowledging shutdown.
    ESP_ERROR_CHECK(xSemaphoreGive(led->task_stopped) == pdTRUE ? ESP_OK : ESP_FAIL);
    vTaskDelete(NULL);
}

/*
BUGS TO FIX:

Other issues I found:

Notifications can prevent blinking: Blinking only happens on timeout. Repeated LED_BLINK notifications less than a second apart continually restart the wait, so the LED never toggles.
Incorrect stack-size comment: 2048 means bytes in ESP-IDF, not words. That doesn’t prove the stack is insufficient, but it’s smaller than your comment suggests.
The current one-time BLINK startup should get past the reported init

*/

esp_err_t led_del(led_t *led)
{
    ESP_RETURN_ON_FALSE(led, ESP_ERR_INVALID_ARG, TAG, "LED must not be NULL.");

    if (led->task_handle)
    {
        BaseType_t ret = xTaskNotify(led->task_handle, LED_TASK_STOP, eSetValueWithOverwrite);
        ESP_RETURN_ON_FALSE(ret == pdPASS, ESP_FAIL, TAG, "Failed to request LED task shutdown.");
        ret = xSemaphoreTake(led->task_stopped, portMAX_DELAY);
        ESP_RETURN_ON_FALSE(ret == pdTRUE, ESP_FAIL, TAG, "Failed to wait for LED task shutdown.");
        led->task_handle = NULL;
    }

    if (led->task_stopped)
    {
        vSemaphoreDelete(led->task_stopped);
        led->task_stopped = NULL;
    }

    if (led->led_handle)
    {
        esp_err_t err = led_strip_del(led->led_handle);
        ESP_RETURN_ON_ERROR(err, TAG, "Failed to delete LED strip.");
        led->led_handle = NULL;
    }

    return ESP_OK;
}

esp_err_t led_init(led_t *led)
{
    if (!led)
    {
        return ESP_ERR_INVALID_ARG;
    }

    *led = (led_t){0};
    led->led_config.strip_gpio_num = BLINK_GPIO;
    led->led_config.max_leds = 1;
    led->rmt_config.resolution_hz = 10 * 1000 * 1000;
    led->rmt_config.flags.with_dma = false;

    esp_err_t err;

    //
    // Initialize the LED strip device.
    //

    err = led_strip_new_rmt_device(&led->led_config, &led->rmt_config, &led->led_handle);
    ESP_RETURN_ON_ERROR(err, TAG, "Failed to create LED strip device.");

    err = led_strip_clear(led->led_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to clear LED strip: %s", esp_err_to_name(err));
        goto cleanup;
    }

    led->task_stopped = xSemaphoreCreateBinary();
    if (!led->task_stopped)
    {
        err = ESP_ERR_NO_MEM;
        ESP_LOGE(TAG, "Failed to create LED shutdown semaphore.");
        goto cleanup;
    }

    //
    // Create a low-priority background task to manage the state of the LED.
    //

    BaseType_t ret;
    ret = xTaskCreate(
        led_task,           // Task function
        "led_task",         // Task name
        2048,               // Stack size (WORDS, not bytes)
        led,                // Task parameter
        tskIDLE_PRIORITY,   // Task priority
        &led->task_handle); // Task handle

    if (ret != pdPASS) {
        err = ESP_FAIL;
        ESP_LOGE(TAG, "Failed to create LED task.");
        goto cleanup;
    }

    return ESP_OK;

cleanup:
    {
        esp_err_t cleanup_err = led_del(led);
        if (cleanup_err != ESP_OK)
        {
            ESP_LOGE(TAG, "LED initialization cleanup failed: %s", esp_err_to_name(cleanup_err));
        }
    }
    return err;
}

esp_err_t led_set_state(led_t *led, enum led_state state)
{
    ESP_RETURN_ON_FALSE(led, ESP_ERR_INVALID_ARG, TAG, "LED must not be NULL.");
    ESP_RETURN_ON_FALSE(state >= LED_OFF && state <= LED_ON_RED, ESP_ERR_INVALID_ARG, TAG, "Invalid LED state.");
    ESP_RETURN_ON_FALSE(led->task_handle, ESP_ERR_INVALID_STATE, TAG, "LED task is not running.");

    BaseType_t ret;
    ret = xTaskNotify(led->task_handle, (uint32_t) state, eSetValueWithOverwrite);
    if (ret != pdPASS)
    {
        return ESP_FAIL;
    }

    return ESP_OK;
}

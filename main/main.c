#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_check.h"
#include "led_strip.h"

static const char *TAG = "blink";

enum {
    BLINK_GPIO = 48,
    BLINK_PERIOD_MS = 5000,
    LED_BRIGHTNESS = 16,
};

//
// LED.h
//

enum led_state {
    LED_OFF,
    LED_ON,
    LED_BLINK,
};

typedef struct {
    led_strip_handle_t led_handle;
    led_strip_config_t led_config;
    led_strip_rmt_config_t rmt_config;
    TaskHandle_t task_handle;
} led_t;


void led_task(void *pvParameters)
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
            ESP_LOGI(TAG, "Notified with value: %u", notified_value);

            // Determin the next state.
            enum led_state next_state = (enum led_state)notified_value;

            if (next_state == current_state)
            {
                // do nothing -- except if state == blink, then we still need to toggle
            }
            else
            {
                switch (next_state)
                {
                case LED_OFF:
                    ESP_ERROR_CHECK(led_strip_clear(led->led_handle));
                    break;

                case LED_ON:
                case LED_BLINK:

                    // TODO: better error handling
                    ESP_ERROR_CHECK(led_strip_set_pixel(led->led_handle, 0, LED_BRIGHTNESS, LED_BRIGHTNESS, LED_BRIGHTNESS));
                    ESP_ERROR_CHECK(led_strip_refresh(led->led_handle));
                    isOn = true;

                    break;
                }

            }
            
            current_state = next_state;

        }
        else
        {
            ESP_LOGI(TAG, "No notification received, toggling if in BLINK state. Current state: %d, LED is currently %s", current_state, isOn ? "ON" : "OFF");

            // Current state is unchanged.
            // Iff current state == BLINK, we need to toggle.
            if (current_state == LED_BLINK)
            {
                if (isOn)
                {
                    ESP_ERROR_CHECK(led_strip_clear(led->led_handle));
                }
                else
                {
                    ESP_ERROR_CHECK(led_strip_set_pixel(led->led_handle, 0, LED_BRIGHTNESS, LED_BRIGHTNESS, LED_BRIGHTNESS));
                    ESP_ERROR_CHECK(led_strip_refresh(led->led_handle));
                }

                isOn = !isOn;
            }
        }
    }
}

/*
BUGS TO FIX:

Other issues I found:

Deletion race: led_del() frees the driver before stopping the task. The task could access the freed handle. Stop the task first, then delete the strip and check the result.
Failure-path resource leaks: If clearing the strip or creating the task fails, the already-created strip isn’t released.
Notifications can prevent blinking: Blinking only happens on timeout. Repeated LED_BLINK notifications less than a second apart continually restart the wait, so the LED never toggles.
Stale state tracking: LED_OFF clears the hardware but doesn’t set isOn = false. Currently, entering ON/BLINK resets it, so this mainly makes the logs inaccurate.
Incorrect stack-size comment: 2048 means bytes in ESP-IDF, not words. That doesn’t prove the stack is insufficient, but it’s smaller than your comment suggests.
The current one-time BLINK startup should get past the reported init

*/

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
    ESP_RETURN_ON_ERROR(err, TAG, "Failed to clear LED strip.");

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
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t led_set_state(led_t *led, enum led_state state)
{
    if (!led)
    {
        return ESP_ERR_INVALID_ARG;
    }

    BaseType_t ret;
    ret = xTaskNotify(led->task_handle, (uint32_t) state, eSetValueWithOverwrite);
    if (ret != pdPASS)
    {
        return ESP_FAIL;
    }

    return ESP_OK;
}

void led_del(led_t *led)
{
    if (led)
    {
        led_strip_del(led->led_handle);

        if (led->task_handle)
        {
            vTaskDelete(led->task_handle);
        }
    }
}


//
// Main - return HR
//

esp_err_t app_run()
{
    //
    // Set the LED state to pulsing white.
    //

    led_t led;
    ESP_RETURN_ON_ERROR(led_init(&led), TAG, "Failed to initialize LED strip.");
    ESP_RETURN_ON_ERROR(led_set_state(&led, LED_BLINK), TAG, "Failed to set LED state.");

    ESP_LOGI(TAG, "Set LED state to BLINK...");

    //
    // Run infinitely.
    //

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

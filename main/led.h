#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "led_strip.h"

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
    SemaphoreHandle_t task_stopped;
} led_t;

// Call these APIs serially from an owning task; keep led alive until deletion.
// Initialize once, then check led_del() before reinitializing or releasing led.
esp_err_t led_init(led_t *led);
esp_err_t led_set_state(led_t *led, enum led_state state);
esp_err_t led_del(led_t *led);

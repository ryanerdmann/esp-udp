#include "esp_log.h"
#include "esp_check.h"
#include "led.h"
#include "net.h"
#include "udp_serv.h"
#include "sdkconfig.h"

static const char *TAG = "blink";

esp_err_t app_run()
{
    //
    // Initialize the LED strip - blink white
    //

    led_t led;
    ESP_RETURN_ON_ERROR(led_init(&led), TAG, "Failed to initialize LED strip.");
    ESP_RETURN_ON_ERROR(led_set_state(&led, LED_BLINK_WHITE), TAG, "Failed to set LED state.");

    ESP_LOGI(TAG, "Set LED state to BLINK_WHITE...");

    //
    // Connect to WIFI
    //

    ESP_RETURN_ON_ERROR(net_init(), TAG, "Failed to initialize networking.");
    ESP_RETURN_ON_ERROR(net_connect(CONFIG_APP_WIFI_SSID, CONFIG_APP_WIFI_PASSWORD),
                        TAG, "Failed to connect to Wi-Fi.");

    //
    // Set LED to solid green
    //

    ESP_RETURN_ON_ERROR(led_set_state(&led, LED_ON_GREEN), TAG, "Failed to set LED state.");


    return udp_server_run();
}

void app_main(void)
{
    ESP_LOGI(TAG, "Running application...");
    ESP_ERROR_CHECK(app_run());
}

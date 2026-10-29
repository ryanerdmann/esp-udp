#include <stdbool.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_check.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "net.h"

static const char *TAG = "net";
static const EventBits_t CONNECTED_BIT = BIT0;
static const EventBits_t FAILED_BIT = BIT1;

static EventGroupHandle_t connection_events;
static bool initialized;
static bool started;
static unsigned int retries;

static void connect_station(void)
{
    esp_err_t err = esp_wifi_connect();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start connection: %s", esp_err_to_name(err));
        xEventGroupSetBits(connection_events, FAILED_BIT);
    }
}

static void net_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START)
    {
        retries = 0;
        connect_station();
    }
    else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED)
    {
        const wifi_event_sta_disconnected_t *event = data;
        xEventGroupClearBits(connection_events, CONNECTED_BIT);
        ESP_LOGW(TAG, "Wi-Fi disconnected (reason %u)", event->reason);
        if (retries < 5)
        {
            ++retries;
            ESP_LOGI(TAG, "Retrying connection (%u/5)", retries);
            connect_station();
        }
        else
        {
            ESP_LOGE(TAG, "Wi-Fi connection retries exhausted");
            xEventGroupSetBits(connection_events, FAILED_BIT);
        }
    }
    else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP)
    {
        const ip_event_got_ip_t *event = data;
        retries = 0;
        ESP_LOGI(TAG, "Connected, IP address: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupClearBits(connection_events, FAILED_BIT);
        xEventGroupSetBits(connection_events, CONNECTED_BIT);
    }
    else if (base == IP_EVENT && id == IP_EVENT_STA_LOST_IP)
    {
        xEventGroupClearBits(connection_events, CONNECTED_BIT);
        ESP_LOGW(TAG, "Station lost its IP address");
    }
}

esp_err_t net_init(void)
{
    ESP_RETURN_ON_FALSE(!initialized && !connection_events, ESP_ERR_INVALID_STATE,
                        TAG, "Network initialization must be called once");
    ESP_RETURN_ON_ERROR(nvs_flash_init(), TAG,
                        "NVS initialization failed; check the NVS partition (not erased automatically)");
    ESP_RETURN_ON_ERROR(esp_netif_init(), TAG, "Failed to initialize network stack");
    ESP_RETURN_ON_ERROR(esp_event_loop_create_default(), TAG, "Failed to create event loop");

    connection_events = xEventGroupCreate();
    ESP_RETURN_ON_FALSE(connection_events, ESP_ERR_NO_MEM, TAG, "Failed to create connection events");
    esp_netif_t *station = esp_netif_create_default_wifi_sta();
    if (!station)
    {
        ESP_LOGE(TAG, "Failed to create station interface");
        vEventGroupDelete(connection_events);
        connection_events = NULL;
        return ESP_ERR_NO_MEM;
    }

    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&config), TAG, "Failed to initialize Wi-Fi");
    // Keep credentials out of NVS; the application supplies them at each boot.
    ESP_RETURN_ON_ERROR(esp_wifi_set_storage(WIFI_STORAGE_RAM), TAG, "Failed to select RAM storage");
    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), TAG, "Failed to select station mode");
    ESP_RETURN_ON_ERROR(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, net_event, NULL),
                        TAG, "Failed to register Wi-Fi handler");
    ESP_RETURN_ON_ERROR(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, net_event, NULL),
                        TAG, "Failed to register IP handler");
    initialized = true;
    return ESP_OK;
}

esp_err_t net_connect(const char *ssid, const char *password)
{
    ESP_RETURN_ON_FALSE(initialized && !started, ESP_ERR_INVALID_STATE,
                        TAG, "Initialize networking first; connection must not already be started");
    ESP_RETURN_ON_FALSE(ssid && password, ESP_ERR_INVALID_ARG, TAG, "SSID and password must not be NULL");

    wifi_config_t config = {0};
    size_t ssid_len = strlen(ssid);
    size_t password_len = strlen(password);
    ESP_RETURN_ON_FALSE(ssid_len > 0 && ssid_len <= sizeof(config.sta.ssid),
                        ESP_ERR_INVALID_ARG, TAG, "SSID must contain 1-32 bytes");
    ESP_RETURN_ON_FALSE(password_len <= sizeof(config.sta.password),
                        ESP_ERR_INVALID_ARG, TAG, "Password must not exceed 64 bytes");
    memcpy(config.sta.ssid, ssid, ssid_len);
    memcpy(config.sta.password, password, password_len);
    config.sta.threshold.authmode = password_len ? WIFI_AUTH_WPA2_PSK : WIFI_AUTH_OPEN;

    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &config), TAG, "Failed to configure station");
    xEventGroupClearBits(connection_events, CONNECTED_BIT | FAILED_BIT);
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "Failed to start Wi-Fi");
    started = true;

    EventBits_t bits = xEventGroupWaitBits(connection_events, CONNECTED_BIT | FAILED_BIT,
                                          pdFALSE, pdFALSE, pdMS_TO_TICKS(30000));
    if ((bits & CONNECTED_BIT) && !(bits & FAILED_BIT))
    {
        return ESP_OK;
    }

    esp_err_t result = bits & FAILED_BIT ? ESP_FAIL : ESP_ERR_TIMEOUT;
    ESP_LOGE(TAG, "Failed to obtain an IP address: %s", esp_err_to_name(result));
    ESP_RETURN_ON_ERROR(esp_wifi_stop(), TAG, "Failed to stop Wi-Fi after connection failure");
    started = false;
    return result;
}

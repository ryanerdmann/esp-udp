#pragma once

#include "esp_err.h"

// Call serially from one task. net_init owns the default event loop and Wi-Fi.
esp_err_t net_init(void);

// Wait up to 30 seconds for an IP address. An empty password selects open Wi-Fi.
// After success, reconnect automatically (up to five retries per disconnection).
// Call again only after a failed connection attempt; reboot to change networks.
esp_err_t net_connect(const char *ssid, const char *password);

#pragma once

#include "esp_err.h"

// Call after Wi-Fi connects. Blocks serving UDP; returns an error on failure.
esp_err_t udp_server_run(void);

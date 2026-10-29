#include <stdbool.h>
#include <errno.h>
#include "lwip/sockets.h"
#include "esp_log.h"
#include "esp_check.h"
#include "udp_serv.h"

static const char *TAG = "udp_serv";

enum {
    UDP_PORT = 3333,
};

esp_err_t udp_server_run(void)
{
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    ESP_RETURN_ON_FALSE(sock >= 0, ESP_FAIL, TAG, "Failed to create UDP socket: errno %d", errno);

    struct sockaddr_in address = {
        .sin_family = AF_INET,
        .sin_port = htons(UDP_PORT),
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };
    if (bind(sock, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        ESP_LOGE(TAG, "Failed to bind UDP port %u: errno %d", UDP_PORT, errno);
        goto cleanup;
    }

    ESP_LOGI(TAG, "UDP ping/pong server listening on port %u", UDP_PORT);
    while (true)
    {
        char buffer[128];
        struct sockaddr_in sender = {0};
        socklen_t sender_len = sizeof(sender);
        int received = recvfrom(sock, buffer, sizeof(buffer), 0,
                                (struct sockaddr *)&sender, &sender_len);
        if (received < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            ESP_LOGE(TAG, "UDP receive failed: errno %d", errno);
            break;
        }

        char sender_ip[INET_ADDRSTRLEN];
        if (!inet_ntop(AF_INET, &sender.sin_addr, sender_ip, sizeof(sender_ip)))
        {
            ESP_LOGE(TAG, "Failed to format UDP sender address: errno %d", errno);
            continue;
        }
        ESP_LOGI(TAG, "Received UDP message from %s:%u", sender_ip, ntohs(sender.sin_port));

        static const char reply[] = "pong";
        int sent = sendto(sock, reply, sizeof(reply) - 1, 0,
                          (struct sockaddr *)&sender, sender_len);
        if (sent < 0)
        {
            ESP_LOGE(TAG, "UDP reply failed: errno %d", errno);
        }
        else if (sent != sizeof(reply) - 1)
        {
            ESP_LOGE(TAG, "Incomplete UDP reply: sent %d bytes", sent);
        }
    }

cleanup:
    if (close(sock) < 0)
    {
        ESP_LOGE(TAG, "Failed to close UDP socket: errno %d", errno);
    }
    return ESP_FAIL;
}

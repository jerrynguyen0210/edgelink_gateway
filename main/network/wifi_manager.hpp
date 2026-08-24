#pragma once

#include "esp_event.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

class WifiManager
{
public:
    WifiManager() = default;
    WifiManager(const WifiManager &) = delete;
    WifiManager &operator=(const WifiManager &) = delete;

    esp_err_t start(const char *ssid, const char *password);
    bool wait_for_connection(TickType_t timeout) const;
    bool is_connected() const;

private:
    static void event_handler(void *context,
                              esp_event_base_t event_base,
                              int32_t event_id,
                              void *event_data);

    EventGroupHandle_t event_group_ = nullptr;
    int retry_count_ = 0;
};

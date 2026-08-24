#pragma once

#include <atomic>

#include "esp_err.h"
#include "mqtt_client.h"

class MqttManager
{
public:
    MqttManager() = default;
    MqttManager(const MqttManager &) = delete;
    MqttManager &operator=(const MqttManager &) = delete;

    esp_err_t start(const char *broker_uri,
                    const char *username,
                    const char *password,
                    const char *topic);
    bool publish(const char *payload) const;
    bool is_connected() const;

private:
    static void event_handler(void *context,
                              esp_event_base_t event_base,
                              int32_t event_id,
                              void *event_data);

    esp_mqtt_client_handle_t client_ = nullptr;
    const char *topic_ = nullptr;
    std::atomic_bool connected_{false};
};

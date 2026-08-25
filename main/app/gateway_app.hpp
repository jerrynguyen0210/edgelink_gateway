#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "network/mqtt_manager.hpp"
#include "network/wifi_manager.hpp"

class GatewayApp
{
public:
    GatewayApp() = default;
    GatewayApp(const GatewayApp &) = delete;
    GatewayApp &operator=(const GatewayApp &) = delete;

    esp_err_t start();

private:
    esp_err_t start_network();
    esp_err_t create_sensor_pipeline();

    QueueHandle_t sensor_queue_ = nullptr;
    WifiManager wifi_manager_;
    MqttManager mqtt_manager_;
};

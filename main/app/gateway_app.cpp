#include "app/gateway_app.hpp"

#include "data/sensor_reading.hpp"
#include "esp_log.h"
#include "tasks/mqtt_task.hpp"
#include "tasks/sensor_task.hpp"

namespace {

constexpr char kLogTag[] = "gateway_app";
constexpr UBaseType_t kSensorQueueLength = 10;

}  // namespace

esp_err_t GatewayApp::start()
{
    ESP_LOGI(kLogTag, "Starting EdgeLink gateway");

    const esp_err_t network_result = start_network();
    if (network_result != ESP_OK) {
        ESP_LOGW(kLogTag, "Network unavailable; starting sensor pipeline offline");
    }

    const esp_err_t pipeline_result = create_sensor_pipeline();
    if (pipeline_result != ESP_OK) {
        ESP_LOGE(kLogTag, "Failed to start sensor pipeline");
        return pipeline_result;
    }

    ESP_LOGI(kLogTag, "EdgeLink gateway started");
    return ESP_OK;
}

esp_err_t GatewayApp::start_network()
{
    esp_err_t result =
        wifi_manager_.start(CONFIG_EDGELINK_WIFI_SSID, CONFIG_EDGELINK_WIFI_PASSWORD);
    if (result != ESP_OK) {
        return result;
    }

    if (!wifi_manager_.wait_for_connection(portMAX_DELAY)) {
        ESP_LOGE(kLogTag, "Wi-Fi connection failed");
        return ESP_FAIL;
    }

    result = mqtt_manager_.start(CONFIG_EDGELINK_MQTT_BROKER_URI,
                                 CONFIG_EDGELINK_MQTT_USERNAME,
                                 CONFIG_EDGELINK_MQTT_PASSWORD,
                                 CONFIG_EDGELINK_MQTT_TOPIC);
    if (result != ESP_OK) {
        ESP_LOGE(kLogTag, "Failed to start MQTT client: %s", esp_err_to_name(result));
    }

    return result;
}

esp_err_t GatewayApp::create_sensor_pipeline()
{
    sensor_queue_ = xQueueCreate(kSensorQueueLength, sizeof(SensorReading));
    if (sensor_queue_ == nullptr) {
        ESP_LOGE(kLogTag, "Failed to create sensor queue");
        return ESP_ERR_NO_MEM;
    }

    if (start_sensor_task(sensor_queue_) != pdPASS) {
        ESP_LOGE(kLogTag, "Failed to create sensor task");
        return ESP_FAIL;
    }

    if (start_mqtt_task(sensor_queue_, mqtt_manager_) != pdPASS) {
        ESP_LOGE(kLogTag, "Failed to create MQTT task");
        return ESP_FAIL;
    }

    return ESP_OK;
}

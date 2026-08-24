#include "data/sensor_reading.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "network/mqtt_manager.hpp"
#include "network/wifi_manager.hpp"
#include "tasks/mqtt_task.hpp"
#include "tasks/sensor_task.hpp"

namespace {

constexpr char kLogTag[] = "sensor";
constexpr UBaseType_t kSensorQueueLength = 10;

QueueHandle_t sensor_queue = nullptr;
WifiManager wifi_manager;
MqttManager mqtt_manager;

}  // namespace

extern "C" void app_main(void)
{
    const esp_err_t wifi_result =
        wifi_manager.start(CONFIG_EDGELINK_WIFI_SSID, CONFIG_EDGELINK_WIFI_PASSWORD);
    if (wifi_result != ESP_OK) {
        ESP_LOGW(kLogTag, "Starting in offline mode");
    } else {
        while (!wifi_manager.is_connected());
        const esp_err_t mqtt_result = mqtt_manager.start(CONFIG_EDGELINK_MQTT_BROKER_URI,
                                                         CONFIG_EDGELINK_MQTT_USERNAME,
                                                         CONFIG_EDGELINK_MQTT_PASSWORD,
                                                         CONFIG_EDGELINK_MQTT_TOPIC);
        if (mqtt_result != ESP_OK) {
            ESP_LOGW(kLogTag, "MQTT is disabled");
        } else {
            while (!mqtt_manager.is_connected());
        }
    }

    sensor_queue = xQueueCreate(kSensorQueueLength, sizeof(SensorReading));
    if (sensor_queue == nullptr) {
        ESP_LOGE(kLogTag, "Failed to create sensor queue");
        return;
    }

    if (start_sensor_task(sensor_queue) != pdPASS ||
        start_mqtt_task(sensor_queue, mqtt_manager) != pdPASS)
    {
        ESP_LOGE(kLogTag, "Failed to create sensor tasks");
    }
}

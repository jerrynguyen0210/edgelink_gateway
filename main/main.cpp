#include "data/sensor_reading.hpp"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "network/wifi_manager.hpp"
#include "network/mqtt_manager.hpp"
#include "tasks/mqtt_task.hpp"

namespace {

constexpr gpio_num_t kLedGpio = GPIO_NUM_2;
constexpr TickType_t kBlinkDelay = pdMS_TO_TICKS(500);
constexpr uint32_t kDeviceId = 1;
constexpr char kLogTag[] = "sensor";
constexpr UBaseType_t kSensorQueueLength = 10;

QueueHandle_t sensor_queue = nullptr;
WifiManager wifi_manager;
MqttManager mqtt_manager;

void sensor_producer_task(void *)
{
    bool led_on = false;
    uint32_t sample_number = 0;

    while (true) {
        led_on = !led_on;
        gpio_set_level(kLedGpio, led_on ? 1 : 0);

        const SensorReading reading =
            make_fake_sensor_reading(kDeviceId, sample_number++);

        if (xQueueSend(sensor_queue, &reading, 0) != pdPASS) {
            ESP_LOGW(kLogTag, "Sensor queue is full; reading dropped");
        }

        vTaskDelay(kBlinkDelay);
    }
}

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

    gpio_reset_pin(kLedGpio);
    gpio_set_direction(kLedGpio, GPIO_MODE_OUTPUT);

    sensor_queue = xQueueCreate(kSensorQueueLength, sizeof(SensorReading));
    if (sensor_queue == nullptr) {
        ESP_LOGE(kLogTag, "Failed to create sensor queue");
        return;
    }

    if (xTaskCreate(sensor_producer_task, "sensor_producer", 3072, nullptr, 5, nullptr) !=
            pdPASS ||
        start_mqtt_task(sensor_queue, mqtt_manager) != pdPASS)
    {
        ESP_LOGE(kLogTag, "Failed to create sensor tasks");
    }
}

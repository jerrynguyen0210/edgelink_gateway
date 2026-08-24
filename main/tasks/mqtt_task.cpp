#include "tasks/mqtt_task.hpp"

#include <cinttypes>
#include <cstdio>

#include "data/sensor_reading.hpp"
#include "esp_log.h"

namespace {

constexpr char kLogTag[] = "mqtt_task";

struct MqttTaskContext
{
    QueueHandle_t queue;
    MqttManager *manager;
};

MqttTaskContext task_context{};

void mqtt_task(void *argument)
{
    const auto *context = static_cast<const MqttTaskContext *>(argument);
    SensorReading reading{};
    char payload[192];

    while (true) {
        if (xQueueReceive(context->queue, &reading, portMAX_DELAY) != pdPASS) {
            continue;
        }

        const int length = std::snprintf(
            payload,
            sizeof(payload),
            "{\"device_id\":%" PRIu32 ",\"type\":\"%s\",\"value\":%.2f,"
            "\"timestamp_ms\":%" PRIu64 ",\"valid\":%s}",
            reading.device_id,
            sensor_type_name(reading.type),
            reading.value,
            reading.timestamp,
            reading.valid ? "true" : "false");

        if (length < 0 || static_cast<size_t>(length) >= sizeof(payload)) {
            ESP_LOGE(kLogTag, "Sensor payload is too large");
            continue;
        }

        ESP_LOGI(kLogTag, "%s", payload);
        if (!context->manager->publish(payload)) {
            ESP_LOGW(kLogTag, "MQTT offline; reading was not published");
        }
    }
}

}  // namespace

BaseType_t start_mqtt_task(QueueHandle_t queue, MqttManager &manager)
{
    if (queue == nullptr) {
        return pdFAIL;
    }

    task_context = MqttTaskContext{.queue = queue, .manager = &manager};
    return xTaskCreate(mqtt_task, "mqtt_publisher", 4096, &task_context, 5, nullptr);
}

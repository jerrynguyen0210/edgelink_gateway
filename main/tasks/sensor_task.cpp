#include "tasks/sensor_task.hpp"

#include "data/sensor_reading.hpp"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/task.h"

namespace {

constexpr char kLogTag[] = "sensor_task";
constexpr gpio_num_t kLedGpio = GPIO_NUM_2;
constexpr TickType_t kSampleInterval = pdMS_TO_TICKS(500);
constexpr uint32_t kDeviceId = 1;

void sensor_task(void *argument)
{
    const QueueHandle_t queue = static_cast<QueueHandle_t>(argument);
    bool led_on = false;
    uint32_t sample_number = 0;

    while (true) {
        led_on = !led_on;
        gpio_set_level(kLedGpio, led_on ? 1 : 0);

        const SensorReading reading =
            make_fake_sensor_reading(kDeviceId, sample_number++);

        if (xQueueSend(queue, &reading, 0) != pdPASS) {
            ESP_LOGW(kLogTag, "Sensor queue is full; reading dropped");
        }

        vTaskDelay(kSampleInterval);
    }
}

}  // namespace

BaseType_t start_sensor_task(QueueHandle_t sensor_queue)
{
    if (sensor_queue == nullptr) {
        return pdFAIL;
    }

    gpio_reset_pin(kLedGpio);
    gpio_set_direction(kLedGpio, GPIO_MODE_OUTPUT);

    return xTaskCreate(
        sensor_task, "sensor_producer", 3072, sensor_queue, 5, nullptr);
}

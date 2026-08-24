#include "data/sensor_reading.hpp"

#include "esp_timer.h"

SensorReading make_fake_sensor_reading(uint32_t device_id, uint32_t sample_number)
{
    const SensorType type = static_cast<SensorType>(sample_number % 4U);
    const float variation = static_cast<float>(sample_number % 10U) / 10.0F;

    float value = 0.0F;
    switch (type) {
        case SensorType::Temperature:
            value = 22.0F + variation;
            break;
        case SensorType::Pressure:
            value = 1012.0F + variation;
            break;
        case SensorType::Voltage:
            value = 3.3F + (variation / 100.0F);
            break;
        case SensorType::Current:
            value = 0.10F + (variation / 100.0F);
            break;
    }

    return SensorReading{
        .device_id = device_id,
        .type = type,
        .value = value,
        .timestamp = static_cast<uint64_t>(esp_timer_get_time()) / 1000U,
        .valid = true,
    };
}

const char *sensor_type_name(SensorType type)
{
    switch (type) {
        case SensorType::Temperature:
            return "temperature";
        case SensorType::Pressure:
            return "pressure";
        case SensorType::Voltage:
            return "voltage";
        case SensorType::Current:
            return "current";
    }

    return "unknown";
}

#pragma once

#include <cstdint>

enum class SensorType
{
    Temperature,
    Pressure,
    Voltage,
    Current
};

struct SensorReading
{
    uint32_t device_id;
    SensorType type;
    float value;
    uint64_t timestamp;
    bool valid;
};

SensorReading make_fake_sensor_reading(uint32_t device_id, uint32_t sample_number);

const char *sensor_type_name(SensorType type);

#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

BaseType_t start_sensor_task(QueueHandle_t sensor_queue);

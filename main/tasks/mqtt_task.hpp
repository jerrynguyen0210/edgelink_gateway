#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "network/mqtt_manager.hpp"

BaseType_t start_mqtt_task(QueueHandle_t queue, MqttManager &manager);

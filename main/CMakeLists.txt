idf_component_register(
    SRCS
        "main.cpp"
        "data/data_manager.cpp"
        "network/wifi_manager.cpp"
        "network/mqtt_manager.cpp"
        "tasks/mqtt_task.cpp"
    INCLUDE_DIRS "."
    PRIV_REQUIRES
        esp_driver_gpio
        esp_timer
        esp_event
        esp_netif
        esp_wifi
        freertos
        nvs_flash
        mqtt
)

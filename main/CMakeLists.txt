idf_component_register(
    SRCS "main.cpp"
    PRIV_REQUIRES fatfs driver esp_driver_i2s esp_driver_i2c esp_driver_gpio
    INCLUDE_DIRS ""
)

idf_component_register(
    SRCS 
        "main.cpp"
        "app/hardware.cpp"
        "state_machine/states/video_player_state.cpp"
    INCLUDE_DIRS "."
    PRIV_REQUIRES fatfs driver esp_driver_i2s esp_driver_i2c esp_driver_gpio
)

#pragma once

#include <driver/gpio.h>

// https://docs.waveshare.com/ESP32-S3-Touch-LCD-1.54#pinout-definition

struct sd_card_gpio {
    static constexpr gpio_num_t clk = GPIO_NUM_16;
    static constexpr gpio_num_t cmd = GPIO_NUM_15;
    static constexpr gpio_num_t d0  = GPIO_NUM_17;
    static constexpr gpio_num_t d1  = GPIO_NUM_18;
    static constexpr gpio_num_t d2  = GPIO_NUM_13;
    static constexpr gpio_num_t d3  = GPIO_NUM_14;
};

struct display_gpio {
    static constexpr gpio_num_t dc   = GPIO_NUM_45;
    static constexpr gpio_num_t cs   = GPIO_NUM_21;
    static constexpr gpio_num_t sck  = GPIO_NUM_38;
    static constexpr gpio_num_t mosi = GPIO_NUM_39;
    static constexpr gpio_num_t rst  = GPIO_NUM_40;
    static constexpr gpio_num_t bl   = GPIO_NUM_46;
};

struct touch_gpio {
    static constexpr gpio_num_t sda  = GPIO_NUM_42;
    static constexpr gpio_num_t scl  = GPIO_NUM_41;
    static constexpr gpio_num_t intt = GPIO_NUM_48;
    static constexpr gpio_num_t rst  = GPIO_NUM_47;
};
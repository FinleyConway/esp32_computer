#include <cstdio>

#include "button.hpp"
#include "sd_card.hpp"

// gpio 0 boot
// gpio 5 power
// gpio 4 plus
button button;

extern "C" void app_main(void) {
    ESP_ERROR_CHECK(button.create());
    ESP_ERROR_CHECK(button.subscribe(button::event::press_down, [] {
        printf("hello\n");
    }));


    sd_card sd_card;
}
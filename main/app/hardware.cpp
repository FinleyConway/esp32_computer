#include "app/hardware.hpp"

void hardware::init() {
        display.init();
        audio.init();
        audio.set_volume(0);

        sd_card.mount();

        boot_button.create(button_gpio::boot);
        power_button.create(button_gpio::power);
        plus_button.create(button_gpio::plus);
    }
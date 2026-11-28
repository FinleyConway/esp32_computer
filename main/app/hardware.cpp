#include "app/hardware.hpp"

void hardware::init() {
        display_driver.init();
        audio_driver.init();
        audio_driver.set_volume(0);

        storage.mount();

        boot_button.create(button_gpio::boot);
        power_button.create(button_gpio::power);
        plus_button.create(button_gpio::plus);
    }
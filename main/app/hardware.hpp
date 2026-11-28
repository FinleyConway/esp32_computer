#pragma once

#include "core/audio.hpp"
#include "core/button.hpp"
#include "core/sd_card.hpp"
#include "core/io/pinout.hpp"
#include "core/display/display.hpp"

struct hardware {
    display display_driver;
    audio audio_driver;

    sd_card storage;

    button boot_button;
    button power_button;
    button plus_button;

    void init();
};
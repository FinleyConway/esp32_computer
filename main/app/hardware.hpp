#pragma once

#include "core/audio.hpp"
#include "core/button.hpp"
#include "core/sd_card.hpp"
#include "core/io/pinout.hpp"
#include "core/display/display.hpp"

struct hardware {
    display display;
    audio audio;

    sd_card sd_card;

    button boot_button;
    button power_button;
    button plus_button;

    void init();
};
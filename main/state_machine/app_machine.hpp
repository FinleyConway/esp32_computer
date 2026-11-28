#pragma once

#include "app/hardware.hpp"
#include "state_machine/machine.hpp"

class video_player_state;

using app_machine = machine<hardware,
    video_player_state
>;
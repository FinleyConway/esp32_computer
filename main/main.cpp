#include "app/hardware.hpp"
#include "state_machine/app_machine.hpp"
#include "state_machine/states/video_player_state.hpp" 

static hardware g_hardware;
static app_machine g_machine{ g_hardware };

extern "C" void app_main(void) {
    g_hardware.init();
    g_machine.set_state<video_player_state>();
}
#include "video_player_state.hpp"

#include "core/audio.hpp"
#include "core/display/display.hpp"

video_player_state::video_player_state(display& display, audio& audio) :
    m_player(display, audio)
{
}

void video_player_state::init(app_machine& machine) {
    m_player.init();
}

void video_player_state::deinit(app_machine& machine) {
    m_player.deinit();
}

void video_player_state::handle_buttons(app_machine& machine) {
    
}
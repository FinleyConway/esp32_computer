#include "state_machine/states/video_player_state.hpp"

video_player_state::video_player_state(hardware& hw) :
    m_player(hw.display_driver, hw.audio_driver)
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
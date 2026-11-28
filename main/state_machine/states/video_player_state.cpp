#include "state_machine/states/video_player_state.hpp"

video_player_state::video_player_state(hardware& hw) :
    m_hw(hw),
    m_player(hw.display_driver, hw.audio_driver)
{
}

void video_player_state::init(app_machine& machine) {
    m_player.init();    

    handle_buttons(machine);

    auto file = m_hw.storage.load_file_to_psram("/sd_card/goat.avi");

    if (file) {
        m_player.play(std::move(file.value()));
    }
}

void video_player_state::deinit(app_machine& machine) {
    m_hw.boot_button.unsubscribe_all();
    m_hw.power_button.unsubscribe_all();
    m_hw.plus_button.unsubscribe_all();

    m_player.deinit();
}

void video_player_state::handle_buttons(app_machine& machine) {
    using ev = button::event;

    auto bind = [this](button& btn, ev event, void (*fn)(void*)) {
        ESP_ERROR_CHECK(btn.subscribe(event, fn, this));
    };

    auto& volume_button     = m_hw.boot_button;
    auto& play_button       = m_hw.power_button;
    auto& video_selc_button = m_hw.plus_button;

    bind(volume_button,     ev::single_click, thunk<&video_player_state::toggle_mute>);
    bind(volume_button,     ev::press_down,   thunk<&video_player_state::enter_volume_mode>);
    bind(volume_button,     ev::press_up,     thunk<&video_player_state::exit_volume_mode>);
    bind(play_button,       ev::press_down,   thunk<&video_player_state::on_play_pressed>);
    bind(video_selc_button, ev::press_down,   thunk<&video_player_state::volume_up>);
}

void video_player_state::toggle_mute() {
    auto& audio = m_hw.audio_driver;

    audio.mute(!audio.is_mute());
}

void video_player_state::on_play_pressed() {
    if (m_volume_mode) {
        volume_down();
    } 
    else if (m_player.playing()) {
        m_player.stop();
    } 
    else {
        m_player.replay();
    }
}

void video_player_state::volume_up() { 
    if (m_volume_mode) change_volume(+c_volume_increment); 
}

void video_player_state::volume_down() {    
    change_volume(-c_volume_increment); 
}

void video_player_state::change_volume(int8_t delta) {
    auto& audio = m_hw.audio_driver;

    audio.set_volume(audio.get_volume() + delta);
}
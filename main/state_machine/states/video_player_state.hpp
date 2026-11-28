#pragma once

#include "state_machine/app_machine.hpp"
#include "video_player/video_player.hpp"

class video_player_state {
public:
    void init(app_machine& machine);

    void deinit(app_machine& machine);

private:
    void handle_buttons(app_machine& machine);
    // void handle_buttons() {
    //     ESP_ERROR_CHECK(volume_button.create(button_gpio::boot));
    //     ESP_ERROR_CHECK(play_button.create(button_gpio::power));
    //     ESP_ERROR_CHECK(video_selc_button.create(button_gpio::plus));

    //     ESP_ERROR_CHECK(volume_button.subscribe(button::event::single_click, [] {
    //         audio.mute(audio.is_mute() ? false : true);
    //     }));
    //     ESP_ERROR_CHECK(volume_button.subscribe(button::event::press_down, [] {
    //         volume_mode = true;
    //     }));
    //     ESP_ERROR_CHECK(volume_button.subscribe(button::event::press_up, [] {
    //         volume_mode = false;
    //     }));

    //     ESP_ERROR_CHECK(play_button.subscribe(button::event::press_down, [] {
    //         if (volume_mode) {
    //             audio.set_volume(audio.get_volume() - volume_increment);
    //         }
    //         else {
    //             if (video_player.playing()) {
    //                 video_player.stop();
    //             }
    //             else {
    //                 video_player.replay();
    //             }
    //         }
    //     }));

    //     ESP_ERROR_CHECK(video_selc_button.subscribe(button::event::press_down, [] {
    //         if (volume_mode) {
    //             audio.set_volume(audio.get_volume() + volume_increment);
    //         }
    //     }));
    // }

private:
    video_player m_player;
    //uint8_t m_volume_increment = 10;
    //bool m_volume_mode = false;
};
#pragma once

#include <atomic>
#include <cstdint>

#include "state_machine/app_machine.hpp"
#include "video_player/video_player.hpp"

class video_player_state {
public:
    explicit video_player_state(hardware& hw);

public:
    void init(app_machine& machine);

    void deinit(app_machine& machine);

private:
    void handle_buttons(app_machine& machine);

    void toggle_mute();
    void enter_volume_mode() { m_volume_mode = true;  }
    void exit_volume_mode()  { m_volume_mode = false; }
    void on_play_pressed();
    void volume_up();
    void volume_down();
    void change_volume(int8_t delta);

    template<void (video_player_state::*Fn)()>
    static void thunk(void* self) {
        (static_cast<video_player_state*>(self)->*Fn)();
    }

private:
    hardware& m_hw;
    video_player m_player;
    std::atomic<bool> m_volume_mode{false};
    
    static constexpr uint8_t c_volume_increment = 10;
};
#include "core/display/display.hpp"
#include "core/display/display_lock.hpp"
#include "core/io/pinout.hpp"
#include "core/button.hpp"
#include "core/sd_card.hpp"
#include "core/audio.hpp"

#include "state_machine/machine.hpp"
#include "state_machine/states/video_player_state.hpp"

// https://github.com/waveshareteam/ESP32-S3-Touch-LCD-1.54/blob/main/examples/ESP32-S3-Touch-LCD-1.54-demo/ESP-IDF-5.5.1/05_lvgl_example/main/main.c

// static void build_ui() {
//     lv_obj_t *scr = lv_screen_active();

//     lv_obj_t *label = lv_label_create(scr);
//     lv_label_set_text(label, "Hello LVGL");
//     lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 30);

//     lv_obj_t *btn = lv_button_create(scr);
//     lv_obj_set_size(btn, 140, 50);
//     lv_obj_center(btn);
//     lv_obj_t *btn_label = lv_label_create(btn);
//     lv_label_set_text(btn_label, "Tap me");
//     lv_obj_center(btn_label);

//     lv_obj_add_event_cb(btn, [](lv_event_t *e) {
//         static int n = 0;
//         lv_obj_t *b = lv_event_get_target_obj(e);
//         lv_label_set_text_fmt(lv_obj_get_child(b, 0), "Taps: %d", ++n);
//     }, LV_EVENT_CLICKED, nullptr);
// }

button play_button;
button volume_button;
button video_selc_button;

sd_card sd_card;

display display;
audio audio;

machine<video_player_state> states;

extern "C" void app_main(void) {
    sd_card.mount();
    display.init();
    audio.init();
    audio.set_volume(0);

    states.set_state<video_player_state>();
}
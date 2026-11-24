#include "core/display/display.hpp"
#include "core/display/display_lock.hpp"
#include "core/io/pinout.hpp"
#include "core/button.hpp"
#include "core/sd_card.hpp"
#include "core/audio.hpp"

#include "video_player/video_player.hpp"
#include <cmath>

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

sd_card sd_card;

display display;
audio audio;
video_player video_player(display, audio);

extern "C" void app_main(void) {
    ESP_ERROR_CHECK(play_button.create(button_gpio::power));
    ESP_ERROR_CHECK(volume_button.create(button_gpio::boot));

    ESP_ERROR_CHECK(play_button.subscribe(button::event::press_down, [] {
        if (video_player.playing()) {
            video_player.stop();
        }
        else {
            video_player.replay();
        }
    }));

    ESP_ERROR_CHECK(volume_button.subscribe(button::event::press_down, [] {
        audio.mute(audio.is_mute() ? false : true);
    }));

    sd_card.mount();

    display.init();
    audio.init();
    audio.set_volume(0);

    video_player.init();

    auto file = sd_card.load_file_to_psram("/sd_card/goat.avi");

    if (file.has_value()) 
        video_player.play(std::move(file.value()));
            
    // display_lock lock;
    // build_ui();
}
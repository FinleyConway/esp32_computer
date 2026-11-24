#pragma once

#include <atomic>

#include <esp_err.h>
#include <avi_player.h>
#include <freertos/task.h>
#include <freertos/FreeRTOS.h>

#include "../core/sd_card.hpp"
#include "../video_player/video_buffer.hpp"

class video_player {
public:
    explicit video_player(display& display) : m_buffer(display) { }

    video_player(const video_player&) = delete;
    video_player& operator=(const video_player&) = delete;

public:
    bool init() {
        if (m_handle != nullptr) return true;  
        if (!m_buffer.init()) return false;

        if (xTaskCreate(&video_player::worker_entry, "vp_worker", 3072, this, 4, &m_worker) != pdPASS) {
            return false;
        }

        avi_player_config_t config = {};
        config.buffer_size     = 64 * 1024;
        config.video_cb        = &video_player::on_video; 
        //config.audio_cb        = &video_player::on_audio;      
        //config.audio_set_clock_cb = &video_player::on_clock;   
        config.avi_play_end_cb = &video_player::on_end;
        config.priority        = 5;
        config.coreID          = 1; 
        config.user_data       = this;
        config.stack_size      = 12 * 1024;
        config.stack_in_psram  = false;

        const esp_err_t err = avi_player_init(config, &m_handle);

        if (err != ESP_OK) {
            ESP_LOGE(c_tag, "avi_player_init failed: %s", esp_err_to_name(err));

            vTaskDelete(m_worker);

            m_handle = nullptr;
            m_worker = nullptr;

            return false;
        }

        m_state = state::idle;

        return true;
    }

    void deinit() {
        if (m_handle != nullptr) {
            stop();
            avi_player_deinit(m_handle);
            m_handle = nullptr;
        }

        if (m_worker != nullptr) {
            vTaskDelete(m_worker);
            m_worker = nullptr;
        }

        m_buffer.deinit();
    }

    bool play(file_data&& video) {
        if (m_handle == nullptr || m_state.load() != state::idle) return false;

        m_video = std::move(video);

        return replay();
    }

    bool replay() {
        if (m_handle == nullptr || m_video.data == nullptr) return false;

        state expected = state::idle;

        if (!m_state.compare_exchange_strong(expected, state::playing)) {
            return false;   // already playing or still stopping
        }

        if (!start_playback()) {
            m_state = state::idle;
            return false;
        }

        return true;
    }

    void stop() {
        if (m_handle == nullptr) return;

        state expected = state::playing;

        if (!m_state.compare_exchange_strong(expected, state::stopping)) return;

        ESP_LOGI(c_tag, "stopping video");

        const esp_err_t err = avi_player_play_stop(m_handle);

        if (err != ESP_OK) {
            ESP_LOGW(c_tag, "stop returned: %s", esp_err_to_name(err));
            m_state = state::idle;   
        }
    }

    bool playing() const { return m_state.load() == state::playing; }
    
private:
    enum class state : uint8_t { idle, playing, stopping };

    bool start_playback() {
        const esp_err_t err = avi_player_play_from_memory(m_handle, m_video.data.get(), m_video.size);

        if (err != ESP_OK) {
            ESP_LOGE(c_tag, "play failed: %s", esp_err_to_name(err));
            return false;
        }
        return true;
    }

    static void on_video(frame_data_t* frame, void* arg) {
        auto& self = *static_cast<video_player*>(arg);

        // drop frames that arrive while stopping or idle
        if (self.m_state.load() != state::playing) return;

        // is not a video
        if (frame->type != FRAME_TYPE_VIDEO) {
            ESP_LOGW(c_tag, "trying to play a non video type");
            return;
        }

        // is not a video of a mjpeg format
        if (frame->video_info.frame_format != FORMAT_MJEPG) {
            ESP_LOGW(c_tag, "trying to play a video that isn't mjpeg");
            return;
        } 

        // give video buffer the jpeg frame to decode and display
        self.m_buffer.on_frame({ frame->data, frame->data_bytes });
    }

    static void on_end(void* arg) {
        auto& self = *static_cast<video_player*>(arg);

        // loop at the start of the video naturally
        if (self.m_state.load() == state::playing) {
            xTaskNotifyGive(self.m_worker);
        }
        // stopped manuallt 
        else {
            self.m_state = state::idle;
        }
    }

    static void worker_entry(void* arg) {
        static_cast<video_player*>(arg)->worker_loop();
    }

    void worker_loop() {
        for (;;) {
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            restart();
        }
    }

    void restart() {
        // give the player task time to return to its ready state
        // the player doesnt give a responce when its ready so this feels hacky
        vTaskDelay(pdMS_TO_TICKS(20));

        if (m_state.load() != state::playing) {
            m_state = state::idle;
            return;
        }

        const esp_err_t err = avi_player_play_from_memory(
            m_handle, m_video.data.get(), m_video.size);

        if (err != ESP_OK) {
            ESP_LOGE(c_tag, "loop restart failed: %s", esp_err_to_name(err));
            m_state = state::idle;  
        }
    }

private:
    static constexpr const char* c_tag = "video_player";

    file_data m_video;
    video_buffer m_buffer;
    TaskHandle_t m_worker = nullptr;
    avi_player_handle_t m_handle = nullptr;
    std::atomic<state> m_state{state::idle};
};
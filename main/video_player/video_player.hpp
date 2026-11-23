#pragma once

#include <atomic>

#include <esp_err.h>
#include <avi_player.h>

#include "../storage/sd_card.hpp"
#include "../video_player/video_buffer.hpp"

class video_player {
public:
    explicit video_player(display& display) : m_buffer(display) { }

    video_player(const video_player&) = delete;
    video_player& operator=(const video_player&) = delete;

public:
    bool init() {
        if (!m_buffer.init()) return false;

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
            return false;
        }

        return true;
    }

    void deinit() {
        if (m_handle != nullptr) {
            stop();
            avi_player_deinit(m_handle);
            m_handle = nullptr;
        }

        m_buffer.deinit();
    }

    bool play(file_data&& video) {
        if (m_handle == nullptr) return false;

        m_video = std::move(video);
        m_playing = true;

        const esp_err_t err = avi_player_play_from_memory(m_handle, m_video.data.get(), m_video.size);
        if (err != ESP_OK) {
            ESP_LOGE(c_tag, "play failed video: %s", esp_err_to_name(err));

            m_playing = false;

            return false;
        }

        return true;
    }

    bool replay() {
        if (m_handle == nullptr) return false;

        m_playing = true;

        const esp_err_t err = avi_player_play_from_memory(m_handle, m_video.data.get(), m_video.size);
        if (err != ESP_OK) {
            ESP_LOGE(c_tag, "play failed video: %s", esp_err_to_name(err));

            m_playing = false;

            return false;
        }

        return true;
    }

    void stop() {
        m_playing = false;         

        if (m_handle != nullptr) avi_player_play_stop(m_handle);   
    }

    bool playing() const { return m_playing; }
    
private:
    static void on_video(frame_data_t* frame, void* arg) {
        auto& self = *static_cast<video_player*>(arg);

        // is not a video where the format is mjpeg
        if (!self.m_playing || frame->type != FRAME_TYPE_VIDEO) {
            ESP_LOGW(c_tag, "trying to play a non video type");
            return;
        }

        if (frame->video_info.frame_format != FORMAT_MJEPG) {
            ESP_LOGW(c_tag, "trying to play a video that isn't mjpeg");
            return;
        } 

        // give video buffer the jpeg frame to decode and display
        self.m_buffer.on_frame({ frame->data, frame->data_bytes });
    }

    static void on_end(void* arg)  {
        auto& self = *static_cast<video_player*>(arg);

        self.m_playing = false;    
        self.replay();
    }

private:
    static constexpr const char* c_tag = "video_player";

    file_data m_video;
    video_buffer m_buffer;
    avi_player_handle_t m_handle = nullptr;
    std::atomic<bool> m_playing = false;
};
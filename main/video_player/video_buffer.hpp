#pragma once

#include <span>
#include <array>
#include <cstddef>
#include <algorithm>

#include <esp_jpeg_dec.h>

#include "../display/display.hpp"

class video_buffer {
public:
    explicit video_buffer(display& display) : m_display(display) {}
    ~video_buffer() { deinit(); }

    video_buffer(const video_buffer&) = delete;
    video_buffer& operator=(const video_buffer&) = delete;

public:
    bool init() {
        constexpr uint8_t bytes_per_pixel = 2;
        constexpr size_t buffer_size = 
                display_info::width * display_info::height * bytes_per_pixel;

        // setup buffers
        for (auto& buffer : m_buffer) {
            buffer = static_cast<uint8_t*>(
                jpeg_calloc_align(buffer_size, 16)
            );

            // failed to create buffer
            if (buffer == nullptr) return false;
        }

        // setup jpeg decoder
        jpeg_dec_config_t config = DEFAULT_JPEG_DEC_CONFIG();
        config.output_type = JPEG_PIXEL_FORMAT_RGB565_BE;

        return jpeg_dec_open(&config, &m_handle) == JPEG_ERR_OK;
    }

    void deinit() {
        if (m_handle) {
            jpeg_dec_close(m_handle);
            m_handle = nullptr;
        }

        for (auto& buffer : m_buffer) {
            if (buffer != nullptr) {
                jpeg_free_align(buffer);
                buffer = nullptr;
            }
        }
    }

    void on_frame(std::span<uint8_t> jpeg) {
        if (m_handle == nullptr) return;

        jpeg_dec_io_t io = {};
        jpeg_dec_header_info_t info = {};
        io.inbuf = jpeg.data();
        io.inbuf_len = jpeg.size();
        
        // get jpeg info
        if (jpeg_dec_parse_header(m_handle, &io, &info) != JPEG_ERR_OK) {
            ESP_LOGW(c_tag, "bad jpeg header");
            return;
        }

        constexpr uint8_t width = display_info::width; 
        constexpr uint8_t height = display_info::height; 
        
        // make sure that video is within the display size
        if (info.width != width || info.height != height) {
            ESP_LOGE(
                c_tag, 
                "video is %dx%d, expected %dx%d or smaller",
                info.width, info.height, width, height
            );
            return;
        }
        

        // get the current buffer
        uint8_t* out = m_buffer[m_buffer_index];
        io.outbuf = out;

        // decode jpeg buffer 
        if (jpeg_dec_process(m_handle, &io) != JPEG_ERR_OK) {
            ESP_LOGW(c_tag, "jpeg decode failed");
            return;
        }

        constexpr uint8_t strip = 50;

        // draw buffer to screen
        for (uint8_t y = 0; y < height; y += strip) {
            const uint8_t end_y = std::min<uint8_t>(y + strip, height);
            constexpr uint8_t bytes_per_pixel = 2;
            const uint8_t* rows = out + y * width * bytes_per_pixel;

            m_display.draw_pixels(0, y, width, end_y, rows);
        }

        // flip flop index to start next buffer
        m_buffer_index ^= 1;
    }

private:
    static constexpr const char* c_tag = "video_buffer";

    display& m_display;
    jpeg_dec_handle_t m_handle = nullptr;
    size_t m_buffer_index = 0;
    std::array<uint8_t*, 2> m_buffer;
};
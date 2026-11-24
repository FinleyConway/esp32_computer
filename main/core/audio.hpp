#pragma once

#include <span>
#include <algorithm>

#include <esp_check.h>
#include <esp_codec_dev.h>
#include <driver/i2s_std.h>
#include <esp_codec_dev_defaults.h>

#include "io/i2c_bus.hpp"
#include "io/pinout.hpp"

class audio {
public:
    audio() = default;
    audio(const audio&) = delete;
    audio& operator=(const audio&) = delete;

public:
    esp_err_t init() {
        i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
        chan_cfg.auto_clear = true; 
        ESP_RETURN_ON_ERROR(i2s_new_channel(&chan_cfg, &m_tx, &m_rx), "audio", "i2s_new_channel");

        i2s_std_config_t std_cfg = {};
        std_cfg.clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(m_rate);
        std_cfg.slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
            I2S_DATA_BIT_WIDTH_16BIT,
            m_channels == 1 ? I2S_SLOT_MODE_MONO : I2S_SLOT_MODE_STEREO
        );
        std_cfg.gpio_cfg.mclk = audio_gpio::mclk;
        std_cfg.gpio_cfg.bclk = audio_gpio::sclk;
        std_cfg.gpio_cfg.ws   = audio_gpio::lrck;
        std_cfg.gpio_cfg.dout = audio_gpio::dsdin;
        std_cfg.gpio_cfg.din  = audio_gpio::asdout;

        ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(m_tx, &std_cfg), "audio", "tx init");
        ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(m_rx, &std_cfg), "audio", "rx init");
        ESP_RETURN_ON_ERROR(i2s_channel_enable(m_tx), "audio", "tx enable");
        ESP_RETURN_ON_ERROR(i2s_channel_enable(m_rx), "audio", "rx enable");

        audio_codec_i2s_cfg_t i2s_if_cfg = {};
        i2s_if_cfg.port      = I2S_NUM_0;
        i2s_if_cfg.tx_handle = m_tx;
        i2s_if_cfg.rx_handle = m_rx;
        auto *data_if = audio_codec_new_i2s_data(&i2s_if_cfg);

        audio_codec_i2c_cfg_t i2c_if_cfg = {};
        i2c_if_cfg.port       = i2c::port;                 
        i2c_if_cfg.addr       = ES8311_CODEC_DEFAULT_ADDR;
        i2c_if_cfg.bus_handle = i2c_bus::get();
        auto *ctrl_if = audio_codec_new_i2c_ctrl(&i2c_if_cfg);
        auto *gpio_if = audio_codec_new_gpio();

        es8311_codec_cfg_t es_cfg = {};
        es_cfg.ctrl_if    = ctrl_if;
        es_cfg.gpio_if    = gpio_if;
        es_cfg.codec_mode = ESP_CODEC_DEV_WORK_MODE_BOTH;
        es_cfg.pa_pin     = audio_gpio::pa_ctrl;
        es_cfg.use_mclk   = true;
        auto *codec_if = es8311_codec_new(&es_cfg);

        esp_codec_dev_cfg_t dev_cfg = {};
        dev_cfg.dev_type = ESP_CODEC_DEV_TYPE_OUT;
        dev_cfg.codec_if = codec_if;
        dev_cfg.data_if  = data_if;
        m_handle = esp_codec_dev_new(&dev_cfg);
        if (!m_handle) return ESP_FAIL;

        esp_codec_dev_sample_info_t fs = {};
        fs.sample_rate     = m_rate;
        fs.channel         = m_channels;
        fs.bits_per_sample = m_bits;
        if (esp_codec_dev_open(m_handle, &fs) != ESP_CODEC_DEV_OK) return ESP_FAIL;

        return ESP_OK;
    }

    esp_err_t reconfigure(uint32_t sample_rate, uint8_t bits, uint8_t channels) {
        if (m_handle == nullptr) return ESP_ERR_INVALID_STATE;

        if (sample_rate == m_rate && bits == m_bits && channels == m_channels) {
            return ESP_OK;  
        }

        esp_codec_dev_close(m_handle);

        esp_codec_dev_sample_info_t fs = {};
        fs.sample_rate     = sample_rate;
        fs.channel         = channels;
        fs.bits_per_sample = bits;

        if (esp_codec_dev_open(m_handle, &fs) != ESP_CODEC_DEV_OK) return ESP_FAIL;

        m_rate = sample_rate; 
        m_bits = bits; 
        m_channels = channels;

        esp_codec_dev_set_out_vol(m_handle, m_volume);

        return ESP_OK;
    }

    void mute(bool mute) {
        if (m_handle == nullptr) return;

        m_is_mute = mute;

        if (mute) {
            esp_codec_dev_set_out_vol(m_handle, 0);
        }
        else {
            esp_codec_dev_set_out_vol(m_handle, m_volume);
        }
    }

    bool is_mute() const {
        return m_is_mute;
    }

    int8_t get_volume() const {
        return m_volume;
    }

    void set_volume(int8_t volume) {
        if (m_handle != nullptr) {
            m_volume = std::clamp<int8_t>(volume, 0, 100);

            esp_codec_dev_set_out_vol(
                m_handle, 
                m_volume
            );
        }
    }

    esp_err_t play_bytes(std::span<uint8_t> bytes) {
        if (m_handle == nullptr) return ESP_ERR_INVALID_STATE;

        const int r = esp_codec_dev_write(
            m_handle,
            bytes.data(),
            bytes.size_bytes()
        );

        return r == ESP_CODEC_DEV_OK ? ESP_OK : ESP_FAIL;
    }

private:
    i2s_chan_handle_t m_tx = nullptr;
    i2s_chan_handle_t m_rx = nullptr;
    esp_codec_dev_handle_t m_handle = nullptr;

    bool m_is_mute = false;
    int8_t m_volume = 50; // default to half volume
    uint32_t m_rate = 16000;
    uint32_t m_bits = 16;
    uint32_t m_channels = 2;

};
#pragma once

#include <string_view>

#include <esp_err.h>
#include <esp_log.h>
#include <esp_vfs_fat.h>
#include <driver/gpio.h>
#include <driver/sdmmc_host.h>

#include "pinout.hpp"

// https://github.com/waveshareteam/ESP32-S3-Touch-LCD-1.54/blob/main/examples/ESP32-S3-Touch-LCD-1.54-demo/ESP-IDF-5.5.1/04_sd_card_test/main/sd_card_example_main.c

class sd_card {
public:
    sd_card() = default;
    ~sd_card() { unmount(); }

    sd_card(const sd_card&) = delete;
    sd_card& operator=(const sd_card&) = delete;

public:
    esp_err_t mount() {
        if (mounted()) return ESP_ERR_INVALID_STATE;

        const esp_vfs_fat_sdmmc_mount_config_t mount_config = {
            .format_if_mount_failed   = false,
            .max_files                = 5,
            .allocation_unit_size     = 0,
            .disk_status_check_enable = false,
            .use_one_fat              = false,
        };

        const sdmmc_host_t host = SDMMC_HOST_DEFAULT();

        sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
        slot.width = 4;
        slot.clk   = sd_card_gpio::clk;
        slot.cmd   = sd_card_gpio::cmd;
        slot.d0    = sd_card_gpio::d0;
        slot.d1    = sd_card_gpio::d1;
        slot.d2    = sd_card_gpio::d2;
        slot.d3    = sd_card_gpio::d3;
        slot.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

        sdmmc_card_t* card = nullptr;

        const esp_err_t err = esp_vfs_fat_sdmmc_mount(
            mount_point().data(), &host, &slot, &mount_config, &card
        );
        
        if (err != ESP_OK) {
            if (err == ESP_FAIL) {
                ESP_LOGE(c_tag, "Failed to mount filesystem (is the card formatted FAT?)");
            } 
            else {
                ESP_LOGE(c_tag, "Failed to initialize card: %s", esp_err_to_name(err));
            }

            return err;
        }

        m_handle = card;

        return ESP_OK;
    }

    esp_err_t unmount() {
        if (!mounted()) return ESP_OK;

        esp_err_t err = esp_vfs_fat_sdcard_unmount(
            mount_point().data(), 
            m_handle
        );

        if (err != ESP_OK) {
            ESP_LOGE(c_tag, "Unmount failed: %s", esp_err_to_name(err));
            return err;
        }

        m_handle = nullptr;

        return err;
    }

    bool mounted() const { 
        return m_handle != nullptr; 
    }

    std::string_view mount_point() const {
        return "/sd_card";
    };

private:
    static constexpr const char* c_tag = "cd_card";

    sdmmc_card_t* m_handle = nullptr;
};
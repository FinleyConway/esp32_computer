#pragma once

#include <memory>
#include <cstdio>
#include <expected>
#include <string_view>

#include <esp_err.h>
#include <esp_log.h>
#include <esp_vfs_fat.h>
#include <driver/gpio.h>
#include <driver/sdmmc_host.h>

#include "io/pinout.hpp"

// https://github.com/waveshareteam/ESP32-S3-Touch-LCD-1.54/blob/main/examples/ESP32-S3-Touch-LCD-1.54-demo/ESP-IDF-5.5.1/04_sd_card_test/main/sd_card_example_main.c

struct psram_deleter {
    void operator()(uint8_t *p) const { heap_caps_free(p); }
};
using psram_buffer = std::unique_ptr<uint8_t[], psram_deleter>;

struct file_closer {
    void operator()(std::FILE *f) const { if (f) std::fclose(f); }
};
using file_ptr = std::unique_ptr<std::FILE, file_closer>;

struct file_data {
    psram_buffer data;
    size_t size = 0;
};

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

public:
    std::expected<file_data, esp_err_t> load_file_to_psram(std::string_view path) {
        if (!mounted()) return std::unexpected(ESP_ERR_INVALID_STATE);

        file_ptr file(std::fopen(path.data(), "rb"));

        // cant open file
        if (file == nullptr) {
            ESP_LOGE(c_tag, "open failed: %s", path.data());
            return std::unexpected(ESP_ERR_NOT_FOUND);
        }

        // get the size of the file
        std::fseek(file.get(), 0, SEEK_END);
        const long size = std::ftell(file.get());
        std::fseek(file.get(), 0, SEEK_SET);

        if (size <= 0) {
            ESP_LOGE(c_tag, "empty or unreadable file: %s", path.data());
            return std::unexpected(ESP_ERR_INVALID_SIZE);
        }

        // allocate a buffer to write into
        psram_buffer buffer(static_cast<uint8_t *>(
            heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT))
        );

        // not enough memory to allocate file into psram
        if (buffer == nullptr) {
            ESP_LOGE(c_tag, "not enough PSRAM for %ld bytes", size);
            return std::unexpected(ESP_ERR_NO_MEM);
        }

        // start reading file into memory
        constexpr size_t chunk_read = 32 * 1024;
        const size_t file_size = static_cast<size_t>(size);
        size_t total = 0;

        while (total < file_size) {
            const size_t want = std::min(chunk_read, file_size - total);
            const size_t got = std::fread(buffer.get() + total, 1, want, file.get());
            if (got == 0) break;
            total += got;
        }

        // check if we have fully copied the data into memory
        if (total != file_size) {
            ESP_LOGE(c_tag, "short read: %u / %u", (unsigned)total, (unsigned)file_size);
            return std::unexpected(ESP_FAIL);
        }

        return file_data{ std::move(buffer), total };
    }

private:
    static constexpr const char* c_tag = "cd_card";

    sdmmc_card_t* m_handle = nullptr;
};
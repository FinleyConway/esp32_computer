#pragma once

#include <cstdint>

#include <driver/i2c_master.h>

#include "pinout.hpp"

struct i2c {
    static constexpr i2c_port_num_t port = I2C_NUM_0;
    static constexpr uint32_t hz         = 400 * 1000;
};

class i2c_bus {
public:
    static i2c_master_bus_handle_t get() {
        static i2c_master_bus_handle_t handle = create(); 
        return handle;
    }

private:
    static i2c_master_bus_handle_t create() {
        i2c_master_bus_config_t cfg = {};
        cfg.i2c_port          = i2c::port;
        cfg.sda_io_num        = i2c_gpio::sda;
        cfg.scl_io_num        = i2c_gpio::scl;
        cfg.clk_source        = I2C_CLK_SRC_DEFAULT;
        cfg.glitch_ignore_cnt = 7;
        cfg.flags.enable_internal_pullup = 1;

        i2c_master_bus_handle_t h = nullptr;
        ESP_ERROR_CHECK(i2c_new_master_bus(&cfg, &h));

        return h;
    }
};
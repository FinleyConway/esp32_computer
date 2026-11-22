#pragma once

#include <esp_log.h>
#include <driver/spi_master.h>
#include <driver/i2c_master.h>

#include <lvgl.h>
#include <esp_lvgl_port.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_st7789.h>
#include <esp_lcd_touch_cst816s.h>

#include "../pinout.hpp"

// https://github.com/waveshareteam/ESP32-S3-Touch-LCD-1.54/blob/main/examples/ESP32-S3-Touch-LCD-1.54-demo/ESP-IDF-5.5.1/05_lvgl_example/main/main.c

struct display_info {
    // screen dimensions
    static constexpr uint16_t width  = 240;
    static constexpr uint16_t height = 240;

    // spi/lcd
    static constexpr spi_host_device_t lcd_host = SPI2_HOST;
    static constexpr uint32_t pixel_clk_hz      = 40 * 1000 * 1000;
    static constexpr uint8_t  lcd_cmd_bits      = 8;
    static constexpr uint8_t  lcd_param_bits    = 8;
    static constexpr uint8_t  spi_queue_depth   = 10;

    // lvgl draw buffers
    static constexpr uint16_t draw_buffer_height = 50;
    static constexpr uint32_t draw_buffer_pixels = width * draw_buffer_height;

    // touch i2c
    //!! gonna need to move this some where else as audio and imu also uses the same bus
    static constexpr i2c_port_num_t touch_i2c_port = I2C_NUM_0;
    static constexpr uint32_t touch_i2c_hz         = 400 * 1000;
};

class display {
public:
    display() = default;
    display(const display &) = delete;
    display &operator=(const display &) = delete;

public:
    void init() {
        init_lcd();
        init_touch();
        init_lvgl();
        set_backlight(true);
    }

    void set_backlight(bool on) {
        gpio_set_level(display_gpio::bl, on); 
    }

    void draw_pixels(uint8_t start_x, uint8_t start_y, uint8_t end_x, uint8_t end_y, const void* data) {
        esp_lcd_panel_draw_bitmap(m_panel, start_x, start_y, end_x, end_y, data);
    }

private:
    void init_lcd() {
        spi_bus_config_t bus = {};
        bus.sclk_io_num     = display_gpio::sck;
        bus.mosi_io_num     = display_gpio::mosi;
        bus.miso_io_num     = GPIO_NUM_NC;
        bus.quadwp_io_num   = GPIO_NUM_NC;
        bus.quadhd_io_num   = GPIO_NUM_NC;
        bus.max_transfer_sz = display_info::draw_buffer_pixels * sizeof(uint16_t);
        ESP_ERROR_CHECK(spi_bus_initialize(display_info::lcd_host, &bus, SPI_DMA_CH_AUTO));

        esp_lcd_panel_io_spi_config_t io_cfg = {};
        io_cfg.dc_gpio_num       = display_gpio::dc;
        io_cfg.cs_gpio_num       = display_gpio::cs;
        io_cfg.pclk_hz           = display_info::pixel_clk_hz;
        io_cfg.lcd_cmd_bits      = display_info::lcd_cmd_bits;
        io_cfg.lcd_param_bits    = display_info::lcd_param_bits;
        io_cfg.spi_mode          = 0;
        io_cfg.trans_queue_depth = display_info::spi_queue_depth;
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(
            (esp_lcd_spi_bus_handle_t)display_info::lcd_host,
            &io_cfg, &m_io)
        );

        esp_lcd_panel_dev_config_t panel_cfg = {};
        panel_cfg.reset_gpio_num = display_gpio::rst;
        panel_cfg.rgb_ele_order  = LCD_RGB_ELEMENT_ORDER_RGB;
        panel_cfg.bits_per_pixel = 16;
        ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(m_io, &panel_cfg, &m_panel));

        ESP_ERROR_CHECK(esp_lcd_panel_reset(m_panel));
        ESP_ERROR_CHECK(esp_lcd_panel_init(m_panel));
        ESP_ERROR_CHECK(esp_lcd_panel_invert_color(m_panel, true));
        ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(m_panel, true));

        gpio_reset_pin(display_gpio::bl);
        gpio_set_direction(display_gpio::bl, GPIO_MODE_OUTPUT);

        set_backlight(false);
    }

    void init_touch() {
        i2c_master_bus_config_t bus_cfg = {};
        bus_cfg.i2c_port          = display_info::touch_i2c_port;
        bus_cfg.sda_io_num        = touch_gpio::sda;
        bus_cfg.scl_io_num        = touch_gpio::scl;
        bus_cfg.clk_source        = I2C_CLK_SRC_DEFAULT;
        bus_cfg.glitch_ignore_cnt = 7;
        bus_cfg.flags.enable_internal_pullup = 1;
        ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &m_i2c_bus));

        esp_lcd_panel_io_i2c_config_t io_cfg = {};
        io_cfg.dev_addr             = ESP_LCD_TOUCH_IO_I2C_CST816S_ADDRESS;
        io_cfg.control_phase_bytes  = 1;
        io_cfg.dc_bit_offset        = 0;
        io_cfg.lcd_cmd_bits         = 8;
        io_cfg.lcd_param_bits       = 0;
        io_cfg.flags.disable_control_phase = 1;
        io_cfg.scl_speed_hz         = display_info::touch_i2c_hz;
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(m_i2c_bus, &io_cfg, &m_tp_io));

        esp_lcd_touch_config_t tp_cfg = {};
        tp_cfg.x_max        = display_info::width;
        tp_cfg.y_max        = display_info::height;
        tp_cfg.rst_gpio_num = touch_gpio::rst;
        tp_cfg.int_gpio_num = touch_gpio::intt;
        ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_cst816s(m_tp_io, &tp_cfg, &m_tp));
    }

    void init_lvgl() {
        const lvgl_port_cfg_t port_cfg = ESP_LVGL_PORT_INIT_CONFIG();
        ESP_ERROR_CHECK(lvgl_port_init(&port_cfg));

        lvgl_port_display_cfg_t d = {};
        d.io_handle     = m_io;
        d.panel_handle  = m_panel;
        d.buffer_size   = display_info::draw_buffer_pixels;
        d.double_buffer = true;
        d.hres          = display_info::width;
        d.vres          = display_info::height;
        d.monochrome    = false;
        d.flags.buff_dma   = true;
        d.flags.swap_bytes = true;  
        m_disp = lvgl_port_add_disp(&d);
        ESP_ERROR_CHECK(m_disp ? ESP_OK : ESP_FAIL);

        lvgl_port_touch_cfg_t t = {};
        t.disp   = m_disp;
        t.handle = m_tp;
        ESP_ERROR_CHECK(lvgl_port_add_touch(&t) ? ESP_OK : ESP_FAIL);
    }

private:
    lv_display_t* m_disp = nullptr;
    esp_lcd_panel_io_handle_t m_io = nullptr;
    esp_lcd_panel_handle_t m_panel = nullptr;
    i2c_master_bus_handle_t m_i2c_bus = nullptr;
    esp_lcd_panel_io_handle_t m_tp_io = nullptr;
    esp_lcd_touch_handle_t m_tp = nullptr;
};
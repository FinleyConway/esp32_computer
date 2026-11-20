#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <esp_err.h>
#include <esp_check.h>

#include <iot_button.h>
#include <button_gpio.h>

class button {
public:
    button() = default;

    button(const button&) = delete;
    button& operator=(const button&) = delete;

public:
    enum class event {
        press_down = BUTTON_PRESS_DOWN,
        press_up = BUTTON_PRESS_UP,
        press_repeat = BUTTON_PRESS_REPEAT,
        press_repeat_done = BUTTON_PRESS_REPEAT_DONE,
        single_click = BUTTON_SINGLE_CLICK,
        double_click = BUTTON_DOUBLE_CLICK,
        multiple_click = BUTTON_MULTIPLE_CLICK,
        long_press_start = BUTTON_LONG_PRESS_START,
        long_press_hold = BUTTON_LONG_PRESS_HOLD,
        long_press_up = BUTTON_LONG_PRESS_UP,
        press_end = BUTTON_PRESS_END,
    };

    using callback = void (*)();

public:
    esp_err_t create() {
        const button_config_t btn_cfg{};
        const button_gpio_config_t gpio_cfg = {
            .gpio_num          = m_gpio, // TODO: Need to pass in gpio
            .active_level      = 0,
            .enable_power_save = true,
            .disable_pull      = false,
        };

        return iot_button_new_gpio_device(&btn_cfg, &gpio_cfg, &m_handle);
    }

    esp_err_t subscribe(event event, callback fn) {
        const auto event_index = static_cast<size_t>(event);

        // recieved an invalid event or null callback
        if (fn == nullptr || event_index >= c_max_events) {
            return ESP_ERR_INVALID_ARG;
        }

        auto& slot = m_events[event_index];
        const bool first = slot == nullptr;
        slot = fn;

        // has the event been registered?
        if (first) {
            // register event 
            const esp_err_t err = iot_button_register_cb(
                m_handle, 
                static_cast<button_event_t>(event), 
                nullptr, 
                &button::on_event, 
                this
            );

            // handle when register event fails
            if (err != ESP_OK) {
                slot = nullptr;

                ESP_RETURN_ON_ERROR(err, c_tag, "failed to subscribe button event");
            }
        }

        return ESP_OK;
    }

private:
    static void on_event(void* args, void* data) {
        const auto& self = *static_cast<button*>(data);
        const auto event_received = iot_button_get_event(static_cast<button_handle_t>(args));
        const auto event_index = static_cast<size_t>(event_received);

        // check if recieved event is valid and call the callback
        if (event_index < c_max_events) {
            if (const auto& event = self.m_events[event_index]; event != nullptr) {
                event();
            }
        }
    }
    
private:
    static constexpr const char* c_tag = "button";
    static constexpr uint8_t c_max_events = BUTTON_EVENT_MAX;

    int32_t m_gpio = 0;
    button_handle_t m_handle = nullptr;
    std::array<callback, c_max_events> m_events{};
};
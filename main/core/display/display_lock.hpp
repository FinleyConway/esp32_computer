#pragma once

#include <cstdint>

#include <esp_lvgl_port.h>

class display_lock {
public:
    explicit display_lock(uint32_t timeout_ms = 0) : m_locked(lvgl_port_lock(timeout_ms)) {}
    ~display_lock() { if (m_locked) lvgl_port_unlock(); }

    display_lock(const display_lock&) = delete;
    display_lock& operator=(const display_lock&) = delete;

public:
    bool is_locked() const {
        return m_locked;
    }

private:
    bool m_locked;
};
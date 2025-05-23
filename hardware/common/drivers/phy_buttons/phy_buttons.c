/*
  phy_buttons.c - Physical buttons driver for PiBot CNC Pendant

  Copyright (c) 2025 Luc Lebosse. All rights reserved.
  Licensed under GNU Lesser General Public License v2.1 or later.
*/

#include "phy_buttons.h"
#include "esp3d_log.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static phy_buttons_config_t buttons_config;
static bool is_initialized = false;

/**
 * @brief Configure the physical buttons
 *
 * This function initializes the GPIO pins for the buttons based on the provided configuration.
 *
 * @param config Pointer to the buttons configuration
 * @return ESP_OK on success, or an error code on failure
 */
esp_err_t phy_buttons_configure(const phy_buttons_config_t *config)
{
    if (config == NULL) {
        esp3d_log_e("Invalid buttons configuration");
        return ESP_ERR_INVALID_ARG;
    }

    memcpy(&buttons_config, config, sizeof(phy_buttons_config_t));

    // Validate GPIO pins
    for (int i = 0; i < 3; i++) {
        if (!GPIO_IS_VALID_GPIO(buttons_config.button_pins[i])) {
            esp3d_log_e("Invalid GPIO number for button %d", i + 1);
            return ESP_ERR_INVALID_ARG;
        }
    }

    // Configure GPIO pins
    for (int i = 0; i < 3; i++) {
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << buttons_config.button_pins[i]),
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = buttons_config.pullups_enabled ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE // Pas d'interruptions pour l'instant
        };
        if (gpio_config(&io_conf) != ESP_OK) {
            esp3d_log_e("Failed to configure GPIO for button %d", i + 1);
            return ESP_ERR_INVALID_ARG;
        }
    }

    is_initialized = true;
    esp3d_log("Physical buttons configured successfully");
    return ESP_OK;
}

/**
 * @brief Read the state of the buttons
 *
 * This function reads the current state of all buttons, applying debounce logic.
 *
 * @param button_states Array to store the state of each button (1 = pressed, 0 = released)
 * @return ESP_OK on success, or an error code on failure
 */
esp_err_t phy_buttons_read(bool button_states[3])
{
    if (!is_initialized) {
        esp3d_log_e("Buttons not configured");
        return ESP_ERR_INVALID_STATE;
    }

    static bool last_states[3] = {0, 0, 0};
    static uint32_t last_change_time[3] = {0, 0, 0};
    uint32_t current_time = esp_timer_get_time() / 1000; // Temps en ms

    for (int i = 0; i < 3; i++) {
        bool current_level = gpio_get_level(buttons_config.button_pins[i]);
        bool current_state = buttons_config.active_low ? !current_level : current_level;

        // Vérification du debounce
        if (current_state != last_states[i]) {
            if ((current_time - last_change_time[i]) >= buttons_config.debounce_ms) {
                last_states[i] = current_state;
                last_change_time[i] = current_time;
                esp3d_log("Button %d state changed to %d", i + 1, current_state);
            }
        }
        button_states[i] = last_states[i];
    }

    return ESP_OK;
}

/*
  disp_backlight.c

  Copyright (c) 2023 Luc Lebosse. All rights reserved.

  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This code is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
*/

#include "disp_backlight.h"

#include <driver/gpio.h>
#include <driver/ledc.h>
#include "esp3d_log.h"

static backlight_config_t backlight_config;
static bool _is_initialized = false;

/**
 * @brief Configures a display backlight instance.
 *
 * This function configures a display backlight instance based on the provided
 * configuration.
 *
 * @param config Pointer to the backlight configuration.
 * @return ESP_OK if the backlight instance is configured successfully, or an
 * error code if it fails.
 */
esp_err_t backlight_configure(const backlight_config_t *config) {
    // Check input parameters
    if (config == NULL) {
        esp3d_log_e("Invalid backlight configuration");
        return ESP_ERR_INVALID_ARG;
    }

    memcpy(&backlight_config, config, sizeof(backlight_config_t));

    if (!GPIO_IS_VALID_OUTPUT_GPIO(backlight_config.gpio_num)) {
        esp3d_log_e("Invalid GPIO number: %d", backlight_config.gpio_num);
        return ESP_ERR_INVALID_ARG;
    }

    esp3d_log_d("Configuring backlight: PWM=%d, GPIO=%d, Invert=%d, Freq=%d Hz, Res=%d bits, Timer=%d, Channel=%d",
              backlight_config.pwm_control, backlight_config.gpio_num, backlight_config.output_invert,
              backlight_config.freq_hz, backlight_config.resolution_bits, backlight_config.timer_idx, backlight_config.channel_idx);

    if (backlight_config.pwm_control) {
        // Configure LED (Backlight) pin as PWM for Brightness control.
        esp3d_log_d("Configuring backlight with PWM control");
        const ledc_channel_config_t LCD_backlight_channel = {
            .gpio_num = backlight_config.gpio_num,
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .channel = backlight_config.channel_idx,
            .intr_type = LEDC_INTR_DISABLE,
            .timer_sel = backlight_config.timer_idx,
            .duty = 0,
            .hpoint = 0,
            .flags.output_invert = backlight_config.output_invert // Apply inversion
        };
        const ledc_timer_config_t LCD_backlight_timer = {
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .duty_resolution = backlight_config.resolution_bits,
            .timer_num = backlight_config.timer_idx,
            .freq_hz = backlight_config.freq_hz,
            .clk_cfg = LEDC_AUTO_CLK
        };

        esp_err_t ret = ledc_timer_config(&LCD_backlight_timer);
        if (ret != ESP_OK) {
            esp3d_log_e("Failed to configure LEDC timer: %d", ret);
            return ret;
        }
        ret = ledc_channel_config(&LCD_backlight_channel);
        if (ret != ESP_OK) {
            esp3d_log_e("Failed to configure LEDC channel: %d", ret);
            return ret;
        }
    } else {
        // Configure GPIO for output
        esp3d_log_d("Configuring backlight with GPIO control");
        esp_rom_gpio_pad_select_gpio(backlight_config.gpio_num);
        if (gpio_set_direction(backlight_config.gpio_num, GPIO_MODE_OUTPUT) != ESP_OK) {
            esp3d_log_e("Failed to set GPIO direction");
            return ESP_ERR_INVALID_ARG;
        }
        // Apply initial GPIO level based on output_invert
        bool initial_level = backlight_config.output_invert ? 0 : 1;
        if (gpio_set_level(backlight_config.gpio_num, initial_level) != ESP_OK) {
            esp3d_log_e("Failed to set initial GPIO level");
            return ESP_ERR_INVALID_ARG;
        }
    }

    esp3d_log_d("Backlight configured successfully");
    _is_initialized = true;
    return ESP_OK;
}

/**
 * @brief Sets the brightness of the display backlight.
 *
 * This function sets the brightness of the display backlight to the specified
 * percentage.
 *
 * @param brightness_percent The brightness level to set, specified as a
 * percentage (0-100).
 *
 * @return ESP_OK if the brightness was set successfully, or an error code if
 * an error occurred.
 */
esp_err_t backlight_set(int brightness_percent) {
    if (!_is_initialized) {
        esp3d_log_e("Backlight not configured");
        return ESP_ERR_INVALID_STATE;
    }

    // Sanitize input
    int brightness = brightness_percent;
    if (brightness > 100) {
        brightness = 100;
    }
    if (brightness < 0) {
        brightness = 0;
    }

    backlight_config.duty = brightness;

    esp3d_log_d("Setting LCD backlight: %d%%", backlight_config.duty);

    if (backlight_config.pwm_control) {
        uint32_t max_duty = (1 << backlight_config.resolution_bits) - 1; // 2^resolution_bits - 1
        uint32_t duty_cycle = (max_duty * backlight_config.duty) / 100;

        esp3d_log_d("PWM: Max duty=%lu, Duty cycle=%lu", max_duty, duty_cycle);

        esp_err_t ret = ledc_set_duty(LEDC_LOW_SPEED_MODE, backlight_config.channel_idx, duty_cycle);
        if (ret != ESP_OK) {
            esp3d_log_e("Failed to set LEDC duty cycle: %d", ret);
            return ret;
        }
        ret = ledc_update_duty(LEDC_LOW_SPEED_MODE, backlight_config.channel_idx);
        if (ret != ESP_OK) {
            esp3d_log_e("Failed to update LEDC duty cycle: %d", ret);
            return ret;
        }
    } else {
        bool gpio_level = (backlight_config.duty > 0) ? !backlight_config.output_invert : backlight_config.output_invert;
        if (gpio_set_level(backlight_config.gpio_num, gpio_level) != ESP_OK) {
            esp3d_log_e("Failed to set GPIO level");
            return ESP_ERR_INVALID_ARG;
        }
    }

    esp3d_log_d("LCD backlight set successfully");
    return ESP_OK;
}

/**
 * @brief Gets the current brightness of the display backlight.
 *
 * This function returns the current brightness level as a percentage.
 *
 * @return The current brightness level (0-100) if initialized, or -1 if not initialized.
 */
int backlight_get_current(void) {
    if (!_is_initialized) {
        esp3d_log_e("Backlight not configured");
        return -1;
    }
    return backlight_config.duty;
}
/*
  buzzer.c - Buzzer driver for ESP3D-TFT

  Copyright (c) 2025 Luc LEBOSSE. All rights reserved.

  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This code is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
*/

#include "buzzer.h"
#include "esp3d_log.h"
#include <driver/gpio.h>
#include <driver/ledc.h>
#include <soc/gpio_sig_map.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_timer.h>
#include <esp_rom_sys.h>

static buzzer_config_t buzzer_config;
static bool _is_initialized = false;

/**
 * @brief Configures the buzzer instance
 *
 * @param config Pointer to the buzzer configuration
 * @return ESP_OK on success, or an error code on failure
 */
esp_err_t buzzer_configure(const buzzer_config_t *config)
{
    if (config == NULL) {
        esp3d_log_e("Invalid buzzer configuration");
        return ESP_ERR_INVALID_ARG;
    }

    memcpy(&buzzer_config, config, sizeof(buzzer_config_t));

    if (!GPIO_IS_VALID_OUTPUT_GPIO(buzzer_config.gpio_num)) {
        esp3d_log_e("Invalid GPIO number");
        return ESP_ERR_INVALID_ARG;
    }

    if (buzzer_config.pwm_control) {
        esp3d_log("Configuring buzzer with PWM control");
        const ledc_channel_config_t buzzer_channel = {
            .gpio_num = buzzer_config.gpio_num,
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .channel = buzzer_config.channel_idx,
            .intr_type = LEDC_INTR_DISABLE,
            .timer_sel = buzzer_config.timer_idx,
            .duty = 0,
            .hpoint = 0
        };
        const ledc_timer_config_t buzzer_timer = {
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .duty_resolution = buzzer_config.resolution_bits,
            .timer_num = buzzer_config.timer_idx,
            .freq_hz = buzzer_config.freq_hz,
            .clk_cfg = LEDC_AUTO_CLK
        };

        if (ledc_timer_config(&buzzer_timer) != ESP_OK) {
            esp3d_log_e("Failed to configure LEDC timer");
            return ESP_ERR_INVALID_ARG;
        }
        if (ledc_channel_config(&buzzer_channel) != ESP_OK) {
            esp3d_log_e("Failed to configure LEDC channel");
            return ESP_ERR_INVALID_ARG;
        }
    } else {
        esp3d_log("Configuring buzzer with GPIO control");
        esp_rom_gpio_pad_select_gpio(buzzer_config.gpio_num);
        if (gpio_set_direction(buzzer_config.gpio_num, GPIO_MODE_OUTPUT) != ESP_OK) {
            esp3d_log_e("Failed to set GPIO direction");
            return ESP_ERR_INVALID_ARG;
        }
        esp_rom_gpio_connect_out_signal(buzzer_config.gpio_num, SIG_GPIO_OUT_IDX,
                                        buzzer_config.output_invert, false);
    }

    _is_initialized = true;
    esp3d_log("Buzzer configured successfully");
    return ESP_OK;
}

/**
 * @brief Plays a single tone on the buzzer
 *
 * @param freq_hz Frequency of the tone in Hz
 * @param duration_ms Duration of the tone in milliseconds
 * @return ESP_OK on success, or an error code on failure
 */
esp_err_t buzzer_bip(uint16_t freq_hz, uint32_t duration_ms)
{
    if (!_is_initialized) {
        esp3d_log_e("Buzzer not configured");
        return ESP_ERR_INVALID_STATE;
    }

    if (freq_hz == 0 || duration_ms == 0) {
        // Silence: turn off buzzer
        if (buzzer_config.pwm_control) {
            ledc_set_duty(LEDC_LOW_SPEED_MODE, buzzer_config.channel_idx, 0);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, buzzer_config.channel_idx);
        } else {
            gpio_set_level(buzzer_config.gpio_num, buzzer_config.output_invert ? 1 : 0);
        }
        return ESP_OK;
    }

    esp3d_log("Playing tone: %d Hz for %ld ms", freq_hz, duration_ms);

    if (buzzer_config.pwm_control) {
        uint32_t max_duty = (1 << buzzer_config.resolution_bits) - 1;
        uint32_t duty_cycle = (max_duty * buzzer_config.duty) / 100;

        // Set frequency
        ledc_set_freq(LEDC_LOW_SPEED_MODE, buzzer_config.timer_idx, freq_hz);
        // Set duty cycle
        ledc_set_duty(LEDC_LOW_SPEED_MODE, buzzer_config.channel_idx, duty_cycle);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, buzzer_config.channel_idx);

        // Wait for duration
        vTaskDelay(pdMS_TO_TICKS(duration_ms));

        // Turn off
        ledc_set_duty(LEDC_LOW_SPEED_MODE, buzzer_config.channel_idx, 0);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, buzzer_config.channel_idx);
    } else {
        // Simple square wave for GPIO control
        uint32_t period_us = 1000000 / freq_hz; // Period in microseconds
        uint32_t half_period_us = period_us / 2;
        uint32_t start_time = esp_timer_get_time() / 1000; // Current time in ms
        uint32_t end_time = start_time + duration_ms;

        while ((esp_timer_get_time() / 1000) < end_time) {
            gpio_set_level(buzzer_config.gpio_num, buzzer_config.output_invert ? 0 : 1);
            esp_rom_delay_us(half_period_us);
            gpio_set_level(buzzer_config.gpio_num, buzzer_config.output_invert ? 1 : 0);
            esp_rom_delay_us(half_period_us);
        }
    }

    esp3d_log("Tone played successfully");
    return ESP_OK;
}

/**
 * @brief Plays a sequence of tones on the buzzer
 *
 * @param tones Array of tones to play
 * @param count Number of tones in the array
 * @return ESP_OK on success, or an error code on failure
 */
esp_err_t buzzer_play(const buzzer_tone_t *tones, uint32_t count)
{
    if (!_is_initialized) {
        esp3d_log_e("Buzzer not configured");
        return ESP_ERR_INVALID_STATE;
    }

    if (tones == NULL || count == 0) {
        esp3d_log_e("Invalid tones array or count");
        return ESP_ERR_INVALID_ARG;
    }

    esp3d_log("Playing %ld tones", count);

    for (uint32_t i = 0; i < count; i++) {
        esp_err_t ret = buzzer_bip(tones[i].freq_hz, tones[i].duration_ms);
        if (ret != ESP_OK) {
            esp3d_log_e("Failed to play tone %ld", i);
            return ret;
        }
        // Small delay between tones to distinguish them
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    esp3d_log("Sequence played successfully");
    return ESP_OK;
}
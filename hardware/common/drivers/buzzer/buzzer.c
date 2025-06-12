/*
  buzzer.c - SC8002B amplifier driver for ESP3D-TFT (PWM tones)

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

#include <driver/gpio.h>
#include <driver/ledc.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include <esp_rom_sys.h>
#include <esp_timer.h>
#include <soc/gpio_sig_map.h>
#include <string.h>

#include "esp3d_log.h"

static buzzer_config_t buzzer_config;
static bool _is_initialized            = false;
static SemaphoreHandle_t _buzzer_mutex = NULL;
static TaskHandle_t _play_task_handle  = NULL;

/**
 * @brief Playback task for non-blocking buzzer_play
 */
static void buzzer_play_task(void *arg)
{
    buzzer_tone_t *tones = (buzzer_tone_t *)arg;
    uint32_t count       = tones[0].duration_ms;  // Store count in first tone's duration_ms
    tones[0].duration_ms = 0;                     // Reset to avoid affecting playback

    esp3d_log("Playback task started with %ld tones", count);

    for (uint32_t i = 0; i < count; i++)
    {
        esp_err_t ret;
        if (xSemaphoreTake(_buzzer_mutex, portMAX_DELAY) == pdTRUE)
        {
            ret = buzzer_bip(tones[i + 1].freq_hz, tones[i + 1].duration_ms);
            xSemaphoreGive(_buzzer_mutex);
        }
        else
        {
            ret = ESP_ERR_TIMEOUT;
        }
        if (ret != ESP_OK)
        {
            esp3d_log_e("Failed to play tone %ld", i);
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(10));  // Small delay between tones
    }

    // Free tones and clean up
    free(tones);
    if (xSemaphoreTake(_buzzer_mutex, portMAX_DELAY) == pdTRUE)
    {
        _play_task_handle = NULL;
        xSemaphoreGive(_buzzer_mutex);
    }
    esp3d_log("Playback task completed");
    vTaskDelete(NULL);
}

/**
 * @brief Configures the buzzer instance
 *
 * @param config Pointer to the buzzer configuration
 * @return ESP_OK on success, or an error code on failure
 */
esp_err_t buzzer_configure(const buzzer_config_t *config)
{
    if (config == NULL)
    {
        esp3d_log_e("Invalid buzzer configuration");
        return ESP_ERR_INVALID_ARG;
    }

    if (_is_initialized)
    {
        esp3d_log_e("Buzzer already configured");
        return ESP_ERR_INVALID_STATE;
    }

    memcpy(&buzzer_config, config, sizeof(buzzer_config_t));

    if (!GPIO_IS_VALID_OUTPUT_GPIO(buzzer_config.gpio_num))
    {
        esp3d_log_e("Invalid GPIO number: %d", buzzer_config.gpio_num);
        return ESP_ERR_INVALID_ARG;
    }

    // Create mutex
    _buzzer_mutex = xSemaphoreCreateMutex();
    if (_buzzer_mutex == NULL)
    {
        esp3d_log_e("Failed to create buzzer mutex");
        return ESP_ERR_NO_MEM;
    }

    esp3d_log("Configuring buzzer with PWM control");
    const ledc_channel_config_t buzzer_channel = {
        .gpio_num            = buzzer_config.gpio_num,
        .speed_mode          = LEDC_LOW_SPEED_MODE,
        .channel             = buzzer_config.channel_idx,
        .intr_type           = LEDC_INTR_DISABLE,
        .timer_sel           = buzzer_config.timer_idx,
        .duty                = 0,
        .hpoint              = 0,
        .flags.output_invert = buzzer_config.output_invert  // Apply inversion
    };
    const ledc_timer_config_t buzzer_timer = {.speed_mode      = LEDC_LOW_SPEED_MODE,
                                              .duty_resolution = buzzer_config.resolution_bits,
                                              .timer_num       = buzzer_config.timer_idx,
                                              .freq_hz         = buzzer_config.freq_hz,
                                              .clk_cfg         = LEDC_AUTO_CLK};

    esp_err_t ret = ledc_timer_config(&buzzer_timer);
    if (ret != ESP_OK)
    {
        vSemaphoreDelete(_buzzer_mutex);
        esp3d_log_e("Failed to configure LEDC timer: %d", ret);
        return ret;
    }
    ret = ledc_channel_config(&buzzer_channel);
    if (ret != ESP_OK)
    {
        vSemaphoreDelete(_buzzer_mutex);
        esp3d_log_e("Failed to configure LEDC channel: %d", ret);
        return ret;
    }

    _is_initialized = true;
    esp3d_log("Buzzer configured successfully");
    return ESP_OK;
}

/**
 * @brief Plays a single tone on the buzzer (blocking)
 *
 * @param freq_hz Frequency of the tone in Hz
 * @param duration_ms Duration of the tone in milliseconds
 * @return ESP_OK on success, or an error code on failure
 */
esp_err_t buzzer_bip(uint16_t freq_hz, uint32_t duration_ms)
{
    if (!_is_initialized)
    {
        esp3d_log_e("Buzzer not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (freq_hz == 0 || duration_ms == 0)
    {
        // Silence: turn off buzzer

        ledc_set_duty(LEDC_LOW_SPEED_MODE, buzzer_config.channel_idx, 0);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, buzzer_config.channel_idx);

        return ESP_OK;
    }

    esp3d_log("Playing tone: %d Hz for %ld ms", freq_hz, duration_ms);

    uint32_t max_duty   = (1 << buzzer_config.resolution_bits) - 1;
    uint32_t duty_cycle = max_duty / 2;  // 50% duty cycle

    esp_err_t ret = ledc_set_freq(LEDC_LOW_SPEED_MODE, buzzer_config.timer_idx, freq_hz);
    if (ret != ESP_OK)
    {
        esp3d_log_e("Failed to set frequency: %d", ret);
        return ret;
    }
    ret = ledc_set_duty(LEDC_LOW_SPEED_MODE, buzzer_config.channel_idx, duty_cycle);
    if (ret != ESP_OK)
    {
        esp3d_log_e("Failed to set duty cycle: %d", ret);
        return ret;
    }
    ret = ledc_update_duty(LEDC_LOW_SPEED_MODE, buzzer_config.channel_idx);
    if (ret != ESP_OK)
    {
        esp3d_log_e("Failed to update duty cycle: %d", ret);
        return ret;
    }

    // Wait for duration
    vTaskDelay(pdMS_TO_TICKS(duration_ms));

    ret = ledc_set_duty(LEDC_LOW_SPEED_MODE, buzzer_config.channel_idx, 0);
    if (ret != ESP_OK)
    {
        esp3d_log_e("Failed to clear duty cycle: %d", ret);
        return ret;
    }
    ret = ledc_update_duty(LEDC_LOW_SPEED_MODE, buzzer_config.channel_idx);
    if (ret != ESP_OK)
    {
        esp3d_log_e("Failed to update clear duty cycle: %d", ret);
        return ret;
    }

    esp3d_log("Tone played successfully");
    return ESP_OK;
}

/**
 * @brief Plays a sequence of tones on the buzzer (non-blocking)
 *
 * @param tones Array of tones to play
 * @param count Number of tones in the array
 * @return ESP_OK on success, or an error code on failure
 */
esp_err_t buzzer_play(const buzzer_tone_t *tones, uint32_t count)
{
    if (!_is_initialized)
    {
        esp3d_log_e("Buzzer not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (tones == NULL || count == 0)
    {
        esp3d_log_e("Invalid tones array or count");
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(_buzzer_mutex, portMAX_DELAY) == pdTRUE)
    {
        if (_play_task_handle != NULL)
        {
            xSemaphoreGive(_buzzer_mutex);
            esp3d_log_e("Playback already in progress");
            return ESP_ERR_INVALID_STATE;
        }

        // Allocate memory for tones copy (count + 1 for count storage)
        buzzer_tone_t *tones_copy = (buzzer_tone_t *)malloc((count + 1) * sizeof(buzzer_tone_t));
        if (tones_copy == NULL)
        {
            xSemaphoreGive(_buzzer_mutex);
            esp3d_log_e("Failed to allocate memory for tones");
            return ESP_ERR_NO_MEM;
        }

        // Copy tones and store count in first tone's duration_ms
        tones_copy[0].duration_ms = count;  // Store count
        memcpy(&tones_copy[1], tones, count * sizeof(buzzer_tone_t));

        // Create playback task
        BaseType_t ret = xTaskCreate(buzzer_play_task,
                                     "buzzer_play",
                                     2048,  // Stack size
                                     tones_copy,
                                     tskIDLE_PRIORITY + 1,  // Priority
                                     &_play_task_handle);

        if (ret != pdPASS)
        {
            free(tones_copy);
            xSemaphoreGive(_buzzer_mutex);
            esp3d_log_e("Failed to create playback task");
            return ESP_ERR_NO_MEM;
        }

        xSemaphoreGive(_buzzer_mutex);
        esp3d_log("Started non-blocking playback of %ld tones", count);
        return ESP_OK;
    }

    esp3d_log_e("Failed to acquire mutex");
    return ESP_ERR_TIMEOUT;
}

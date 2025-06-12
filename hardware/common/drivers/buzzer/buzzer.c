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
#include <driver/dac_cosine.h>
#include <soc/gpio_sig_map.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <esp_timer.h>
#include <esp_rom_sys.h>
#include <string.h>

static buzzer_config_t buzzer_config;
static bool _is_initialized = false;
static bool _is_loud = false;
static SemaphoreHandle_t _buzzer_mutex = NULL;
static TaskHandle_t _play_task_handle = NULL;
static dac_cosine_handle_t dac_handle = NULL;
static bool _dac_running = false;
static uint16_t _current_freq_hz = 1000; // Track current frequency

// Configure DAC cosine generator with specified frequency and loudness
static esp_err_t configure_dac_cosine(uint16_t freq_hz) {
    dac_cosine_config_t cos_cfg = {
        .chan_id = DAC_CHAN_1, // GPIO26 (DAC2)
        .freq_hz = freq_hz, // Set desired frequency
        .clk_src = DAC_COSINE_CLK_SRC_DEFAULT,
        .atten = _is_loud ? DAC_COSINE_ATTEN_DB_0 : DAC_COSINE_ATTEN_DB_6, // Loud: 0 dB, Normal: -3 dB
        .phase = DAC_COSINE_PHASE_0,
        .offset = 0,
    };

    esp_err_t ret = dac_cosine_new_channel(&cos_cfg, &dac_handle);
    if (ret != ESP_OK) {
        esp3d_log_e("Failed to create cosine channel: %s", esp_err_to_name(ret));
        return ret;
    }
    _current_freq_hz = freq_hz;
    return ESP_OK;
}

// Start DAC cosine if not running
static esp_err_t start_dac_cosine(void) {
    if (!_dac_running) {
        esp_err_t ret = dac_cosine_start(dac_handle);
        if (ret != ESP_OK) {
            esp3d_log_e("Failed to start cosine wave: %s", esp_err_to_name(ret));
            return ret;
        }
        _dac_running = true;
    }
    return ESP_OK;
}

// Stop DAC cosine
static void stop_dac_cosine(void) {
    if (dac_handle != NULL && _dac_running) {
        dac_cosine_stop(dac_handle);
        _dac_running = false;
    }
}

// Clean up DAC cosine channel
static void cleanup_dac_cosine(void) {
    if (dac_handle != NULL) {
        stop_dac_cosine();
        dac_cosine_del_channel(dac_handle);
        dac_handle = NULL;
    }
}

/**
 * @brief Playback task for non-blocking buzzer_play
 */
static void buzzer_play_task(void *arg)
{
    buzzer_tone_t *tones = (buzzer_tone_t *)arg;
    uint32_t count = tones[0].duration_ms;
    tones[0].duration_ms = 0;

    esp3d_log("Playback task started with %ld tones", count);

    for (uint32_t i = 0; i < count; i++) {
        esp_err_t ret;
        if (xSemaphoreTake(_buzzer_mutex, portMAX_DELAY) == pdTRUE) {
            ret = buzzer_bip(tones[i + 1].freq_hz, tones[i + 1].duration_ms);
            xSemaphoreGive(_buzzer_mutex);
        } else {
            ret = ESP_ERR_TIMEOUT;
        }
        if (ret != ESP_OK) {
            esp3d_log_e("Failed to play tone %ld", i);
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    // Stop DAC only after all tones are played
    if (xSemaphoreTake(_buzzer_mutex, portMAX_DELAY) == pdTRUE) {
        if (buzzer_config.waveform == WAVEFORM_SINE && buzzer_config.gpio_num == GPIO_NUM_26) {
            stop_dac_cosine();
        }
        xSemaphoreGive(_buzzer_mutex);
    }

    free(tones);
    if (xSemaphoreTake(_buzzer_mutex, portMAX_DELAY) == pdTRUE) {
        _play_task_handle = NULL;
        xSemaphoreGive(_buzzer_mutex);
    }
    esp3d_log("Playback task completed");
    vTaskDelete(NULL);
}

/**
 * @brief Configures the buzzer instance
 */
esp_err_t buzzer_configure(const buzzer_config_t *config)
{
    if (config == NULL) {
        esp3d_log_e("Invalid buzzer configuration");
        return ESP_ERR_INVALID_ARG;
    }

    if (_is_initialized) {
        esp3d_log_e("Buzzer already configured");
        return ESP_ERR_INVALID_STATE;
    }

    memcpy(&buzzer_config, config, sizeof(buzzer_config_t));

    if (!GPIO_IS_VALID_OUTPUT_GPIO(buzzer_config.gpio_num)) {
        esp3d_log_e("Invalid GPIO number");
        return ESP_ERR_INVALID_ARG;
    }

    _buzzer_mutex = xSemaphoreCreateMutex();
    if (_buzzer_mutex == NULL) {
        esp3d_log_e("Failed to create buzzer mutex");
        return ESP_ERR_NO_MEM;
    }

    if (buzzer_config.waveform == WAVEFORM_SINE && buzzer_config.gpio_num == GPIO_NUM_26) {
        esp3d_log("Configuring buzzer with DAC cosine for sine wave");
        if (configure_dac_cosine(buzzer_config.freq_hz) != ESP_OK) {
            vSemaphoreDelete(_buzzer_mutex);
            esp3d_log_e("Failed to configure DAC cosine");
            return ESP_ERR_INVALID_ARG;
        }
    } else if (buzzer_config.pwm_control) {
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
            vSemaphoreDelete(_buzzer_mutex);
            esp3d_log_e("Failed to configure LEDC timer");
            return ESP_ERR_INVALID_ARG;
        }
        if (ledc_channel_config(&buzzer_channel) != ESP_OK) {
            vSemaphoreDelete(_buzzer_mutex);
            esp3d_log_e("Failed to configure LEDC channel");
            return ESP_ERR_INVALID_ARG;
        }
    } else {
        esp3d_log("Configuring buzzer with GPIO control");
        esp_rom_gpio_pad_select_gpio(buzzer_config.gpio_num);
        if (gpio_set_direction(buzzer_config.gpio_num, GPIO_MODE_OUTPUT) != ESP_OK) {
            vSemaphoreDelete(_buzzer_mutex);
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
 * @brief Sets the loudness mode for the buzzer
 */
esp_err_t buzzer_set_loud(bool loud)
{
    if (!_is_initialized) {
        esp3d_log_e("Buzzer not configured");
        return ESP_ERR_INVALID_STATE;
    }

    if (xSemaphoreTake(_buzzer_mutex, portMAX_DELAY) == pdTRUE) {
        _is_loud = loud;
        if (buzzer_config.waveform == WAVEFORM_SINE && buzzer_config.gpio_num == GPIO_NUM_26 && dac_handle != NULL) {
            // Reconfigure DAC to update attenuation
            cleanup_dac_cosine();
            esp_err_t ret = configure_dac_cosine(_current_freq_hz);
            if (ret != ESP_OK) {
                xSemaphoreGive(_buzzer_mutex);
                return ret;
            }
            if (_dac_running) {
                ret = start_dac_cosine();
                if (ret != ESP_OK) {
                    xSemaphoreGive(_buzzer_mutex);
                    return ret;
                }
            }
        }
        esp3d_log("Buzzer loud mode set to: %s", loud ? "loud" : "normal");
        xSemaphoreGive(_buzzer_mutex);
        return ESP_OK;
    }

    esp3d_log_e("Failed to acquire mutex");
    return ESP_ERR_TIMEOUT;
}

/**
 * @brief Plays a single tone on the buzzer (blocking)
 */
esp_err_t buzzer_bip(uint16_t freq_hz, uint32_t duration_ms)
{
    if (!_is_initialized) {
        esp3d_log_e("Buzzer not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (freq_hz == 0 || duration_ms == 0) {
        if (buzzer_config.waveform == WAVEFORM_SINE && buzzer_config.gpio_num == GPIO_NUM_26) {
            stop_dac_cosine();
        } else if (buzzer_config.pwm_control) {
            ledc_set_duty(LEDC_LOW_SPEED_MODE, buzzer_config.channel_idx, 0);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, buzzer_config.channel_idx);
        } else {
            gpio_set_level(buzzer_config.gpio_num, buzzer_config.output_invert ? 1 : 0);
        }
        return ESP_OK;
    }

    uint8_t duty = _is_loud ? buzzer_config.loud_duty : buzzer_config.duty;
    esp3d_log("Playing tone: %d Hz for %ld ms with %s mode (duty: %d%%)", 
              freq_hz, duration_ms, _is_loud ? "loud" : "normal", duty);

    if (buzzer_config.waveform == WAVEFORM_SINE && buzzer_config.gpio_num == GPIO_NUM_26) {
        // Reconfigure DAC with new frequency
        cleanup_dac_cosine();
        esp_err_t ret = configure_dac_cosine(freq_hz);
        if (ret != ESP_OK) {
            return ret;
        }
        // Start cosine wave
        ret = start_dac_cosine();
        if (ret != ESP_OK) {
            return ret;
        }
        // Wait for duration
        vTaskDelay(pdMS_TO_TICKS(duration_ms));
        // Stop cosine wave
        stop_dac_cosine();
        return ESP_OK;
    } else if (buzzer_config.pwm_control) {
        uint32_t max_duty = (1 << buzzer_config.resolution_bits) - 1;
        uint32_t duty_cycle = (max_duty * duty) / 100;

        ledc_set_freq(LEDC_LOW_SPEED_MODE, buzzer_config.timer_idx, freq_hz);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, buzzer_config.channel_idx, duty_cycle);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, buzzer_config.channel_idx);

        vTaskDelay(pdMS_TO_TICKS(duration_ms));

        ledc_set_duty(LEDC_LOW_SPEED_MODE, buzzer_config.channel_idx, 0);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, buzzer_config.channel_idx);
    } else {
        uint32_t period_us = 1000000 / freq_hz;
        uint32_t half_period_us = period_us / 2;
        uint32_t on_time_us = (half_period_us * duty) / 100;
        uint32_t off_time_us = half_period_us - on_time_us;
        uint32_t start_time = esp_timer_get_time() / 1000;
        uint32_t end_time = start_time + duration_ms;

        while ((esp_timer_get_time() / 1000) < end_time) {
            if (on_time_us > 0) {
                gpio_set_level(buzzer_config.gpio_num, buzzer_config.output_invert ? 0 : 1);
                esp_rom_delay_us(on_time_us);
            }
            if (off_time_us > 0) {
                gpio_set_level(buzzer_config.gpio_num, buzzer_config.output_invert ? 1 : 0);
                esp_rom_delay_us(off_time_us);
            }
        }
    }

    esp3d_log("Tone played successfully");
    return ESP_OK;
}

/**
 * @brief Plays a sequence of tones on the buzzer (non-blocking)
 */
esp_err_t buzzer_play(const buzzer_tone_t *tones, uint32_t count)
{
    if (!_is_initialized) {
        esp3d_log_e("Buzzer not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (tones == NULL || count == 0) {
        esp3d_log_e("Invalid tones array or count");
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(_buzzer_mutex, portMAX_DELAY) == pdTRUE) {
        if (_play_task_handle != NULL) {
            xSemaphoreGive(_buzzer_mutex);
            esp3d_log_e("Playback already in progress");
            return ESP_ERR_INVALID_STATE;
        }

        buzzer_tone_t *tones_copy = (buzzer_tone_t *)malloc((count + 1) * sizeof(buzzer_tone_t));
        if (tones_copy == NULL) {
            xSemaphoreGive(_buzzer_mutex);
            esp3d_log_e("Failed to allocate memory for tones");
            return ESP_ERR_NO_MEM;
        }

        tones_copy[0].duration_ms = count;
        memcpy(&tones_copy[1], tones, count * sizeof(buzzer_tone_t));

        BaseType_t ret = xTaskCreate(
            buzzer_play_task,
            "buzzer_play",
            2048,
            tones_copy,
            tskIDLE_PRIORITY + 1,
            &_play_task_handle
        );

        if (ret != pdPASS) {
            free(tones_copy);
            xSemaphoreGive(_buzzer_mutex);
            esp3d_log_e("Failed to create playback task");
            return ESP_ERR_NO_MEM;
        }

        xSemaphoreGive(_buzzer_mutex);
        esp3d_log("Started non-blocking playback of %ld tones with %s mode", count, _is_loud ? "loud" : "normal");
        return ESP_OK;
    }

    esp3d_log_e("Failed to acquire mutex");
    return ESP_ERR_TIMEOUT;
}
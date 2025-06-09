/*
  buzzer.h - Buzzer driver for ESP3D-TFT

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

#pragma once

#include <esp_err.h>
#include "buzzer_config.h"
#include "buzzer_tone.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configures the buzzer instance
 *
 * @param config Pointer to the buzzer configuration
 * @return ESP_OK on success, or an error code on failure
 */
esp_err_t buzzer_configure(const buzzer_config_t *config);

/**
 * @brief Sets the loudness mode for the buzzer
 *
 * @param loud True for loud mode (50% duty, louder), false for normal mode (100% duty, quieter)
 * @return ESP_OK on success, or an error code on failure
 */
esp_err_t buzzer_set_loud(bool loud);

/**
 * @brief Plays a single tone on the buzzer (blocking)
 *
 * @param freq_hz Frequency of the tone in Hz
 * @param duration_ms Duration of the tone in milliseconds
 * @return ESP_OK on success, or an error code on failure
 */
esp_err_t buzzer_bip(uint16_t freq_hz, uint32_t duration_ms);

/**
 * @brief Plays a sequence of tones on the buzzer (non-blocking)
 *
 * Starts a background task to play the sequence, returning immediately.
 * Only one sequence can play at a time; subsequent calls return ESP_ERR_INVALID_STATE if a sequence is active.
 *
 * @param tones Array of tones to play
 * @param count Number of tones in the array
 * @return ESP_OK on success, or an error code on failure
 */
esp_err_t buzzer_play(const buzzer_tone_t *tones, uint32_t count);

#ifdef __cplusplus
} /* extern "C" */
#endif
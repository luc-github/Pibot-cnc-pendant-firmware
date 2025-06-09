/*
  buzzer_tone.h - Buzzer tone for buzzer driver for ESP3D-TFT

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
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Tone structure for buzzer playback
 */
typedef struct {
    uint16_t freq_hz;    // Frequency in Hz
    uint32_t duration_ms; // Duration in milliseconds
} buzzer_tone_t;

#ifdef __cplusplus
} /* extern "C" */
#endif
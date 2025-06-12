/*
  buzzer_config.h - Buzzer configuration for ESP3D-TFT

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
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configuration structure of buzzer controller
 *
 * Must be passed to buzzer_configure() for correct configuration
 */
typedef struct {
    bool pwm_control;         // true: LEDC is used, false: GPIO is used
    bool output_invert;       // true: GPIO is inverted, false: GPIO is not inverted
    gpio_num_t gpio_num;      // GPIO pin for buzzer
    int timer_idx;            // LEDC timer index
    int channel_idx;          // LEDC channel index
    uint16_t freq_hz;         // Default PWM frequency in Hz
    uint8_t resolution_bits;  // PWM resolution in bits
    uint8_t duty;             // Default duty cycle for normal mode (0-100%)
    uint8_t loud_duty;        // Duty cycle for loud mode (0-100%)
} buzzer_config_t;

#ifdef __cplusplus
} /* extern "C" */
#endif
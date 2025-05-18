/*
  backlight_config.h

  Copyright (c) 2022 Luc Lebosse. All rights reserved.

  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This code is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#pragma once
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configuration structure of backlight controller
 *
 * Must be passed to backlight_configure() for correct configuration
 */
typedef struct {
  bool pwm_control;    // true: LEDC is used, false: GPIO is used
  bool output_invert;  // true: GPIO is inverted, false: GPIO is not inverted
  gpio_num_t gpio_num;        // see gpio_num_t

  // Relevant only for PWM controlled backlight
  // Ignored for switch (ON/OFF) backlight control
  int timer_idx;    // ledc_timer_t
  int channel_idx;  // ledc_channel_t
  uint8_t duty;     // duty of pwm
  uint16_t freq_hz;  // frequency of pwm
  uint8_t resolution_bits;  // resolution of pwm
} backlight_config_t;




#ifdef __cplusplus
} /* extern "C" */
#endif
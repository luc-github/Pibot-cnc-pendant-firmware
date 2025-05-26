/*
  buzzer_def.h - Buzzer default definitions for ESP3D-TFT

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
#include "board_config.h"
#include "buzzer_config.h"

#ifdef __cplusplus
extern "C" {
#endif

buzzer_config_t buzzer_cfg = {
    .pwm_control = BUZZER_PWM_ENABLED_FLAG,
    .output_invert = !BUZZER_ACTIVE_HIGH_FLAG, // Invert if not active-high
    .gpio_num = BUZZER_PIN,
    .timer_idx = BUZZER_PWM_TIMER_IDX,
    .channel_idx = BUZZER_PWM_CHANNEL_IDX,
    .freq_hz = BUZZER_PWM_FREQ_HZ,
    .resolution_bits = BUZZER_PWM_RESOLUTION_BITS,
    .duty = BUZZER_DEFAULT_DUTY_PCT,
    .loud_duty = BUZZER_LOUD_DUTY_PCT
};

#ifdef __cplusplus
} /* extern "C" */
#endif
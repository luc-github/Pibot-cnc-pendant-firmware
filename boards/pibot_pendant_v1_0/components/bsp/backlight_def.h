/*
  backlight_def.h - Backlight screen definitions for ESP3D-TFT

  Copyright (c) 2025 Luc LEBOSSE. All rights reserved.

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

#ifdef __cplusplus
extern "C" {
#endif

 backlight_config_t backlight_cfg = {  .pwm_control = BACKLIGHT_PWM_ENABLED_FLAG,
                                            .output_invert = !(BACKLIGHT_ACTIVE_HIGH_FLAG),
                                            .gpio_num = BACKLIGHT_PIN,
                                            .timer_idx = BACKLIGHT_PWM_TIMER_IDX,
                                            .channel_idx = 0,
                                            .duty=BACKLIGHT_DEFAULT_LEVEL_PCT,
                                            .freq_hz = BACKLIGHT_PWM_FREQ_HZ,
                                            .resolution_bits = BACKLIGHT_PWM_RESOLUTION_BITS};

#ifdef __cplusplus
} /* extern "C" */
#endif

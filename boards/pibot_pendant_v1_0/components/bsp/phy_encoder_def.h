/*
  phy_encoder_def.h - Rotary encoder definitions for PiBot CNC Pendant
  Copyright (c) 2025 Luc Lebosse. All rights reserved.
  Licensed under GNU Lesser General Public License v2.1 or later.
*/

#pragma once

#include "board_config.h"
#include "phy_encoder_config.h"

#ifdef __cplusplus
extern "C" {
#endif

phy_encoder_config_t phy_encoder_cfg = {
    .pin_a = ENCODER_A_PIN,
    .pin_b = ENCODER_B_PIN,
    .pullups_enabled = ENCODER_PULLUPS_ENABLED_FLAG,
    .debounce_us = ENCODER_DEBOUNCE_US,
    .min_step_interval_us = ENCODER_MIN_STEP_INTERVAL_US,
    .steps_per_rev = ENCODER_STEPS_PER_REV_NB,
    .pcnt_high_limit = ENCODER_PCNT_HIGH_LIMIT,
    .pcnt_low_limit = ENCODER_PCNT_LOW_LIMIT,
    .pcnt_glitch_ns = ENCODER_PCNT_GLITCH_NS
};

#ifdef __cplusplus
} /* extern "C" */
#endif
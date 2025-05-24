/*
  phy_switch_def.h - 4-position switch definitions for PiBot CNC Pendant
  Copyright (c) 2025 Luc Lebosse. All rights reserved.
  Licensed under GNU Lesser General Public License v2.1 or later.
*/

#pragma once

#include "board_config.h"
#include "phy_switch_config.h"

#ifdef __cplusplus
extern "C" {
#endif

phy_switch_config_t phy_switch_cfg = {
    .pins = {SWITCH_POS_1_PIN, SWITCH_POS_2_PIN, SWITCH_POS_3_PIN},
    .pullups_enabled = SWITCH_PULLUPS_ENABLED_FLAG,
    .debounce_ms = SWITCH_DEBOUNCE_MS_TIME 
};

#ifdef __cplusplus
} /* extern "C" */
#endif

/*
  phy_buttons_def.h - Physical buttons definitions for PiBot CNC Pendant
  Copyright (c) 2025 Luc Lebosse. All rights reserved.
  Licensed under GNU Lesser General Public License v2.1 or later.
*/

#pragma once

#include "board_config.h"
#include "phy_buttons_config.h"

#ifdef __cplusplus
extern "C" {
#endif

phy_buttons_config_t phy_buttons_cfg = {
    .button_pins = {BUTTON_1_PIN, BUTTON_2_PIN, BUTTON_3_PIN},
    .pullups_enabled = 1, // Pull-ups internes activés (comme INPUT_PULLUP dans Arduino)
    .debounce_ms = BUTTON_DEBOUNCE_MS_TIME, // 20 ms
    .active_low = 1 // Boutons actifs à l'état bas (pressé = 0)
};

#ifdef __cplusplus
} /* extern "C" */
#endif

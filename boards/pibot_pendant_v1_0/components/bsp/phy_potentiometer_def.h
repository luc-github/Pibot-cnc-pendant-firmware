/*
  phy_potentiometer_def.h - Potentiometer definitions for PiBot CNC Pendant
  Copyright (c) 2025 Luc Lebosse. All rights reserved.
  Licensed under GNU Lesser General Public License v2.1 or later.
*/

#pragma once
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "board_config.h"
#include "phy_potentiometer_config.h"

#ifdef __cplusplus
extern "C" {
#endif

phy_potentiometer_config_t phy_potentiometer_cfg = {
    .pin = ANALOG_INPUT_PIN,
    .channel = ANALOG_ADC_CHANNEL_IDX,
    .width = ANALOG_ADC_WIDTH_BITS,
    .atten = ADC_ATTEN_DB_12,
    .filter_enabled = ANALOG_FILTER_ENABLED_FLAG,
    .filter_samples = ANALOG_FILTER_SAMPLES_NB
};

#ifdef __cplusplus
} /* extern "C" */
#endif
/*
  phy_potentiometer_config.h - Potentiometer configuration for PiBot CNC Pendant
  Copyright (c) 2025 Luc Lebosse. All rights reserved.
  Licensed under GNU Lesser General Public License v2.1 or later.
*/

#pragma once

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configuration structure for potentiometer
 *
 * Must be passed to phy_potentiometer_configure() for correct configuration
 */
typedef struct {
    gpio_num_t pin;              // GPIO pin for ADC input
    adc_channel_t channel;       // ADC channel
    adc_bitwidth_t width;        // ADC resolution (bits)
    adc_atten_t atten;           // ADC attenuation
    bool filter_enabled;         // true: Enable filtering, false: Disable
    uint32_t filter_samples;     // Number of samples for filtering
} phy_potentiometer_config_t;

#ifdef __cplusplus
} /* extern "C" */
#endif
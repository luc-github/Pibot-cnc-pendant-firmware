/*
  phy_potentiometer.c - Potentiometer driver for PiBot CNC Pendant
  Copyright (c) 2025 Luc Lebosse. All rights reserved.
  Licensed under GNU Lesser General Public License v2.1 or later.
*/

#include "phy_potentiometer.h"
#include "esp3d_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static phy_potentiometer_config_t pot_config;
static bool is_initialized = false;
static adc_cali_handle_t adc_cali_handle = NULL;
static adc_oneshot_unit_handle_t adc1_handle = NULL;

/**
 * @brief Configure the potentiometer
 */
esp_err_t phy_potentiometer_configure(const phy_potentiometer_config_t *config)
{
    if (config == NULL) {
        esp3d_log_e("Invalid potentiometer configuration");
        return ESP_ERR_INVALID_ARG;
    }

    memcpy(&pot_config, config, sizeof(phy_potentiometer_config_t));

    // Validate GPIO pin and ADC channel
    if (!GPIO_IS_VALID_GPIO(pot_config.pin)) {
        esp3d_log_e("Invalid GPIO number for potentiometer pin");
        return ESP_ERR_INVALID_ARG;
    }

    // Initialize ADC oneshot unit
    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    esp_err_t ret = adc_oneshot_new_unit(&init_cfg, &adc1_handle);
    if (ret != ESP_OK) {
        esp3d_log_e("Failed to initialize ADC unit: %d", ret);
        return ret;
    }

    // Configure ADC channel
    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = pot_config.atten,
        .bitwidth = pot_config.width,
    };
    ret = adc_oneshot_config_channel(adc1_handle, pot_config.channel, &chan_cfg);
    if (ret != ESP_OK) {
        esp3d_log_e("Failed to configure ADC channel: %d", ret);
        adc_oneshot_del_unit(adc1_handle);
        return ret;
    }

    // Initialize ADC calibration
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT_1,
        .atten = pot_config.atten,
        .bitwidth = pot_config.width,
        .default_vref = 1100,
    };
    ret = adc_cali_create_scheme_line_fitting(&cali_config, &adc_cali_handle);
    if (ret != ESP_OK) {
        esp3d_log_e("Failed to create ADC calibration scheme: %d", ret);
        adc_oneshot_del_unit(adc1_handle);
        return ret;
    }

    is_initialized = true;
    esp3d_log("Potentiometer configured successfully");
    return ESP_OK;
}

/**
 * @brief Read the potentiometer value
 */
esp_err_t phy_potentiometer_read(uint32_t *value)
{
    if (!is_initialized) {
        esp3d_log_e("Potentiometer not configured");
        return ESP_ERR_INVALID_STATE;
    }

    if (value == NULL) {
        esp3d_log_e("Invalid value pointer");
        return ESP_ERR_INVALID_ARG;
    }

    uint32_t adc_sum = 0;
    uint32_t samples = pot_config.filter_enabled ? pot_config.filter_samples : 1;

    // Lire plusieurs échantillons et faire la moyenne
    for (uint32_t i = 0; i < samples; i++) {
        int raw;
        esp_err_t ret = adc_oneshot_read(adc1_handle, pot_config.channel, &raw);
        if (ret != ESP_OK) {
            esp3d_log_e("Failed to read ADC: %d", ret);
            return ret;
        }
        adc_sum += raw;
        vTaskDelay(pdMS_TO_TICKS(2)); // Petit délai entre lectures
    }

    // Calculer la moyenne
    *value = adc_sum / samples;

    // Convertir la valeur brute en tension pour débogage
    int voltage;
    esp_err_t ret = adc_cali_raw_to_voltage(adc_cali_handle, *value, &voltage);
    if (ret != ESP_OK) {
        esp3d_log_e("Failed to convert ADC raw to voltage: %d", ret);
        return ret;
    }
    //esp3d_log_d("Potentiometer: raw=%ld, voltage=%d mV", *value, voltage);

    return ESP_OK;
}
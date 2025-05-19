/*
  disp_backlight.c

  Copyright (c) 2023 Luc Lebosse. All rights reserved.

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

/*********************
 *      INCLUDES
 *********************/
#include "disp_backlight.h"

#include <driver/gpio.h>
#include <driver/ledc.h>
#include <soc/gpio_sig_map.h>
#include <soc/ledc_periph.h>  // to invert LEDC output on IDF version < v4.3

#include "esp3d_log.h"
backlight_config_t * backlight_config = NULL;

/**
 * @brief Creates a display backlight instance.
 *
 * This function creates a display backlight instance based on the provided
 * configuration.
 *
 * @param config Pointer to the backlight configuration.
 * @return `ESP_OK` if the backlight instance is created successfully, or an
 * error code if it fails.
 */
esp_err_t backlight_configure( backlight_config_t *config) {
  // Check input parameters
  if (config == NULL) {
    esp3d_log_e("Invalid backlight configuration");
    return ESP_ERR_INVALID_ARG;
  }
   backlight_config = config;
  if (!GPIO_IS_VALID_OUTPUT_GPIO(backlight_config->gpio_num)) {
    esp3d_log_e("Invalid GPIO number");
    return ESP_ERR_INVALID_ARG;
  }
  if (backlight_config->pwm_control) {
    // Configure LED (Backlight) pin as PWM for Brightness control.
    esp3d_log("Configuring backlight with PWM control");
    const ledc_channel_config_t LCD_backlight_channel = {
        .gpio_num = backlight_config->gpio_num,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = backlight_config->channel_idx,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = backlight_config->timer_idx,
        .duty = 0,
        .hpoint = 0};
    const ledc_timer_config_t LCD_backlight_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = backlight_config->resolution_bits,
        .timer_num = backlight_config->timer_idx,
        .freq_hz = backlight_config->freq_hz,
        .clk_cfg = LEDC_AUTO_CLK};

    if (ledc_timer_config(&LCD_backlight_timer) != ESP_OK) {
      esp3d_log_e("Failed to configure LEDC timer");
      return ESP_ERR_INVALID_ARG;
    }
    if (ledc_channel_config(&LCD_backlight_channel) != ESP_OK) {
      esp3d_log_e("Failed to configure LEDC channel");
      return ESP_ERR_INVALID_ARG;
    }
    esp_rom_gpio_connect_out_signal(
        backlight_config->gpio_num,
        ledc_periph_signal[LEDC_LOW_SPEED_MODE].sig_out0_idx +
            backlight_config->channel_idx,
        backlight_config->output_invert, 0);
  } else {
    // Configure GPIO for output
    esp3d_log("Configuring backlight with GPIO control");
    esp_rom_gpio_pad_select_gpio(backlight_config->gpio_num);
    if (gpio_set_direction(backlight_config->gpio_num, GPIO_MODE_OUTPUT) != ESP_OK) {
      esp3d_log_e("Failed to set GPIO direction");
      return ESP_ERR_INVALID_ARG;
    }
    esp_rom_gpio_connect_out_signal(backlight_config->gpio_num, SIG_GPIO_OUT_IDX,
                                    backlight_config->output_invert, false);

    esp3d_log("Backlight created successfully");
  }

  return ESP_OK;
}

/**
 * @brief Sets the brightness of the display backlight.
 *
 * This function sets the brightness of the display backlight to the specified
 * percentage.
 * @param brightness_value The brightness level to set, specified as a
 * percentage (0-100). or 0 /1 for on/off
 *
 * @return `ESP_OK` if the brightness was set successfully, or an error code if
 * an error occurred.
 */
esp_err_t backlight_set(int brightness_value) {
  if (backlight_config == NULL) {
    esp3d_log_e("Backlight not configured");
    return ESP_ERR_INVALID_STATE;
  }
  // Do some sanity checks
  int brightness = brightness_value;
  if (brightness > 100) {
    brightness = 100;
  }
  if (brightness < 0) {
    brightness = 0;
  }

  backlight_config->duty = brightness;

  esp3d_log("Setting LCD backlight: %d%%", backlight_config->duty);
  // Apply the brightness
  if (backlight_config->pwm_control) {
    //max  duty calculation
    uint32_t max_duty = (1 << backlight_config->resolution_bits) - 1;  // 2^resolution_bits - 1
    
    // duty calculation according to the resolution
    uint32_t duty_cycle = (max_duty * backlight_config->duty) / 100;
    
    if (ledc_set_duty(LEDC_LOW_SPEED_MODE, backlight_config->channel_idx, duty_cycle) != ESP_OK) {
      esp3d_log_e("Failed to set LEDC duty cycle");
      return ESP_ERR_INVALID_ARG;
    }
    if (ledc_update_duty(LEDC_LOW_SPEED_MODE, backlight_config->channel_idx) != ESP_OK) {
      esp3d_log_e("Failed to update LEDC duty cycle");
      return ESP_ERR_INVALID_ARG;
    }
  } else {
    // for GPIO, value > 0 is ON
    bool gpio_level = backlight_config->duty > 0 ? 1 : 0;
    if (gpio_set_level(backlight_config->gpio_num, gpio_level) != ESP_OK) {
      esp3d_log_e("Failed to set GPIO level");
      return ESP_ERR_INVALID_ARG;
    }
  }
  esp3d_log("LCD backlight set successfully");
  return ESP_OK;
}



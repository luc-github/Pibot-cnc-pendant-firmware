/*
  touch_ft6336u.c

  Copyright (c) 2024 Luc Lebosse. All rights reserved.

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

#include "touch_ft6336u.h"
#include "esp3d_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include <string.h>

/* Register definitions */
#define FT6336U_DEVICE_MODE       0x00
#define FT6336U_GESTURE_ID        0x01
#define FT6336U_TOUCH_POINTS      0x02
#define FT6336U_TOUCH1_XH         0x03
#define FT6336U_TOUCH1_XL         0x04
#define FT6336U_TOUCH1_YH         0x05
#define FT6336U_TOUCH1_YL         0x06
#define FT6336U_THRESHHOLD        0x80
#define FT6336U_MONITOR_MODE      0x86
#define FT6336U_LIB_VERSION_H     0xA1
#define FT6336U_LIB_VERSION_L     0xA2
#define FT6336U_CHIP_ID           0xA3
#define FT6336U_INTERRUPT_MODE    0xA4
#define FT6336U_FIRMWARE_VERSION  0xA6
#define FT6336U_VENDOR_ID         0xA8

/* Static variables */
static touch_ft6336u_config_t _config;
static bool _is_initialized = false;
static uint16_t _x_max = 0;
static uint16_t _y_max = 0;
static volatile bool _touch_interrupt = false;

/* ISR handler for touch interrupt */
static void IRAM_ATTR touch_ft6336u_touch_isr_handler(void *arg) {
    _touch_interrupt = true;
}

/* I2C communication functions */
static esp_err_t touch_ft6336u_i2c_read(uint8_t reg, uint8_t *data, size_t len) {
    if (!_is_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    esp_err_t ret;

    // Write register address
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (_config.i2c_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);

    // Read data
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (_config.i2c_addr << 1) | I2C_MASTER_READ, true);
    if (len > 1) {
        i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);

    ret = i2c_master_cmd_begin(_config.i2c_port, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    return ret;
}

static esp_err_t touch_ft6336u_i2c_write(uint8_t reg, const uint8_t *data, size_t len) {
    if (!_is_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    esp_err_t ret;

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (_config.i2c_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    if (len > 0) {
        i2c_master_write(cmd, (uint8_t *)data, len, true);
    }
    i2c_master_stop(cmd);

    ret = i2c_master_cmd_begin(_config.i2c_port, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    return ret;
}

static esp_err_t touch_ft6336u_read_byte(uint8_t reg, uint8_t *data) {
    return touch_ft6336u_i2c_read(reg, data, 1);
}

static esp_err_t touch_ft6336u_write_byte(uint8_t reg, uint8_t data) {
    return touch_ft6336u_i2c_write(reg, &data, 1);
}

static bool touch_ft6336u_detect_touch(void) {
    uint8_t touch_points = 0;
    esp_err_t ret = touch_ft6336u_read_byte(FT6336U_TOUCH_POINTS, &touch_points);
    
    if (ret != ESP_OK) {
        return false;
    }
    
    return (touch_points > 0 && touch_points < 6); // FT6336U supports up to 5 touch points
}

/* Public functions */
esp_err_t touch_ft6336u_init(const touch_ft6336u_config_t *config) {
    if (_is_initialized) {
        esp3d_log("FT6336U already initialized");
        return ESP_OK;
    }

    if (config == NULL) {
        esp3d_log_e("Invalid configuration");
        return ESP_ERR_INVALID_ARG;
    }

    // Copy configuration
    memcpy(&_config, config, sizeof(touch_ft6336u_config_t));

    // Store max values
    _x_max = config->x_max;
    _y_max = config->y_max;

    esp3d_log("FT6336U init with x_max=%d, y_max=%d", _x_max, _y_max);

    // Configure I2C
    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = _config.sda_pin,
        .scl_io_num = _config.scl_pin,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = config->i2c_clk_speed,
    };



    // Initialize I2C
    esp_err_t ret = i2c_param_config(config->i2c_port, &i2c_conf);
    if (ret != ESP_OK) {
        esp3d_log_e("Failed to configure I2C: %d", ret);
        return ret;
    }

    ret = i2c_driver_install(config->i2c_port, I2C_MODE_MASTER, 0, 0, 0);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        esp3d_log_e("Failed to install I2C driver: %d", ret);
        return ret;
    }

    // Configure INT pin if specified
    if (GPIO_IS_VALID_GPIO(config->int_pin)) {
        esp3d_log("Configuring INT pin: %d", config->int_pin);
        gpio_config_t int_gpio_cfg = {
            .pin_bit_mask = 1ULL << config->int_pin,
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_NEGEDGE,  // Falling edge
        };
        
        ret = gpio_config(&int_gpio_cfg);
        if (ret != ESP_OK) {
            esp3d_log_e("Failed to configure INT pin: %d", ret);
            return ret;
        }

        // Install ISR service and handler
        ret = gpio_install_isr_service(0);
        if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
            esp3d_log_e("Failed to install ISR service: %d", ret);
            // Continue anyway, as this might fail if the service is already installed
        }

        ret = gpio_isr_handler_add(config->int_pin, touch_ft6336u_touch_isr_handler, NULL);
        if (ret != ESP_OK) {
            esp3d_log_e("Failed to add ISR handler: %d", ret);
            return ret;
        }
    }

    // Configure RST pin and perform reset if specified
    if (GPIO_IS_VALID_GPIO(config->rst_pin)) {
        esp3d_log("Configuring RST pin: %d", config->rst_pin);
        gpio_config_t rst_gpio_cfg = {
            .pin_bit_mask = 1ULL << config->rst_pin,
            .mode = GPIO_MODE_OUTPUT,
        };
        
        ret = gpio_config(&rst_gpio_cfg);
        if (ret != ESP_OK) {
            esp3d_log_e("Failed to configure RST pin: %d", ret);
            return ret;
        }

        // Perform hardware reset
        gpio_set_level(config->rst_pin, 0);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level(config->rst_pin, 1);
        vTaskDelay(pdMS_TO_TICKS(50));  // Wait for FT6336U to initialize
    }

    // Read chip information
    uint8_t vendor_id = 0;
    uint8_t chip_id = 0;
    uint8_t firmware_version = 0;
    uint8_t lib_version_h = 0;
    uint8_t lib_version_l = 0;

    ret = touch_ft6336u_read_byte(FT6336U_VENDOR_ID, &vendor_id);
     if (ret != ESP_OK) {
        esp3d_log_e("Failed to vendor information: %d", ret);
        vendor_id = 0x11;  // Valeur typique
     }
    ret |= touch_ft6336u_read_byte(FT6336U_CHIP_ID, &chip_id);
    if (ret != ESP_OK) {
        esp3d_log_e("Failed to read chip ID: %d", ret);
        chip_id = 0x36;    // FT6336U
    }
    ret |= touch_ft6336u_read_byte(FT6336U_FIRMWARE_VERSION, &firmware_version);
    if (ret != ESP_OK) {
        esp3d_log_e("Failed to read firmware version: %d", ret);
    }
    ret |= touch_ft6336u_read_byte(FT6336U_LIB_VERSION_H, &lib_version_h);
    if (ret != ESP_OK) {
        esp3d_log_e("Failed to read library version high: %d", ret);
    }
    ret |= touch_ft6336u_read_byte(FT6336U_LIB_VERSION_L, &lib_version_l);
    if (ret != ESP_OK) {
        esp3d_log_e("Failed to read library version low: %d", ret);
    }

    if (ret != ESP_OK) {
        esp3d_log_e("Failed to read chip information: %d", ret);
     
           
       // return ret;
    }

    esp3d_log("FT6336U found with Vendor ID: 0x%02x, Chip ID: 0x%02x, Firmware: 0x%02x, Library: 0x%02x 0x%02x",
              vendor_id, chip_id, firmware_version, lib_version_h, lib_version_l);

    // Configure touch controller settings
    // Set threshold
    ret = touch_ft6336u_write_byte(FT6336U_THRESHHOLD, 40);  // Default threshold value
    if (ret != ESP_OK) {
        esp3d_log_e("Failed to set threshold: %d", ret);
    }

    // Set to monitoring mode
    ret = touch_ft6336u_write_byte(FT6336U_MONITOR_MODE, 0);  // 0 = Active mode
    if (ret != ESP_OK) {
        esp3d_log_e("Failed to set monitor mode: %d", ret);
    }

    // Set interrupt mode
    ret = touch_ft6336u_write_byte(FT6336U_INTERRUPT_MODE, 0);  // 0 = Polling mode, 1 = Interrupt trigger mode
    if (ret != ESP_OK) {
        esp3d_log_e("Failed to set interrupt mode: %d", ret);
    }

    _is_initialized = true;
    esp3d_log("FT6336U initialized successfully");
    return ESP_OK;
}

touch_ft6336u_data_t touch_ft6336u_read(void) {
    touch_ft6336u_data_t data = {
        .is_pressed = false,
        .x = -1,
        .y = -1
    };

    if (!_is_initialized) {
        return data;
    }

    if (touch_ft6336u_detect_touch()) {
        uint8_t touch_data[4];
        esp_err_t ret = touch_ft6336u_i2c_read(FT6336U_TOUCH1_XH, touch_data, 4);
        
        if (ret != ESP_OK) {
            esp3d_log_e("Failed to read touch data: %d", ret);
            return data;
        }

        data.is_pressed = true;
        data.x = ((touch_data[0] & 0x0F) << 8) | touch_data[1];
        data.y = ((touch_data[2] & 0x0F) << 8) | touch_data[3];

        // Apply transformations
        if (_config.swap_xy) {
            int16_t temp = data.x;
            data.x = data.y;
            data.y = temp;
        }

        if (_config.invert_x) {
            data.x = _x_max - data.x;
        }

        if (_config.invert_y) {
            data.y = _y_max - data.y;
        }

        esp3d_log("Touch detected at (%d, %d)", data.x, data.y);
    }

    return data;
}

uint16_t touch_ft6336u_get_x_max(void) {
    return _x_max;
}

uint16_t touch_ft6336u_get_y_max(void) {
    return _y_max;
}
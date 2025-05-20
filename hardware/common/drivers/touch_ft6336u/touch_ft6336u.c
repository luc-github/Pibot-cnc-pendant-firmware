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
#define FT6336U_TOUCHRATE_ACTIVE  0x88

#define ACK_CHECK_EN              0x1
#define ACK_CHECK_DIS             0x0

#define FT6336U_VENDID            0x11
#define FT6336U_CHIPID            0x36

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

/* Communication fonctions I2C modifiées avec séquence STOP/START */
static esp_err_t touch_ft6336u_read_byte(uint8_t reg, uint8_t *data) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    
    // Envoi de l'adresse du registre
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (_config.i2c_addr << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(_config.i2c_port, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    if (ret != ESP_OK) {
        return ret;
    }
    
    // Lecture des données
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (_config.i2c_addr << 1) | I2C_MASTER_READ, ACK_CHECK_EN);
    i2c_master_read_byte(cmd, data, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    
    ret = i2c_master_cmd_begin(_config.i2c_port, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    return ret;
}

static esp_err_t touch_ft6336u_write_byte(uint8_t reg, uint8_t data) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (_config.i2c_addr << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, data, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(_config.i2c_port, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    return ret;
}

/* Fonction pour lire un bloc de données de registres consécutifs */
static esp_err_t touch_ft6336u_read_data(uint8_t *data, size_t len) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    
    // Positionnement sur le registre 0
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (_config.i2c_addr << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, 0, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(_config.i2c_port, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    if (ret != ESP_OK) {
        return ret;
    }
    
    // Lecture des données
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (_config.i2c_addr << 1) | I2C_MASTER_READ, ACK_CHECK_EN);
    if (len > 1) {
        i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    
    ret = i2c_master_cmd_begin(_config.i2c_port, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    return ret;
}

/* Test simple de ping du périphérique */
static esp_err_t touch_ft6336u_ping(void) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (_config.i2c_addr << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(_config.i2c_port, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    return ret;
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

    // Attendre que le bus I2C soit stable
    vTaskDelay(pdMS_TO_TICKS(200));

    // Test ping simple pour vérifier si le périphérique répond
    ret = touch_ft6336u_ping();
    if (ret != ESP_OK) {
        esp3d_log_e("FT6336U ping failed: %d", ret);
        // Continuer malgré l'échec
    } else {
        esp3d_log("FT6336U ping successful, device detected at 0x%02x", _config.i2c_addr);
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

        // Perform hardware reset with longer delays
        gpio_set_level(config->rst_pin, 0);
        vTaskDelay(pdMS_TO_TICKS(20));  // Augmenter à 20ms
        gpio_set_level(config->rst_pin, 1);
        vTaskDelay(pdMS_TO_TICKS(300));  // Augmenter à 300ms pour être sûr
    }

    // Initialisation simplifiée comme dans le driver FT6X36.cpp
    
    // Set device mode (normal operation)
    ret = touch_ft6336u_write_byte(FT6336U_DEVICE_MODE, 0x00);
    if (ret != ESP_OK) {
        esp3d_log_e("Failed to set device mode: %d", ret);
    }
    
    // Set threshold
    ret = touch_ft6336u_write_byte(FT6336U_THRESHHOLD, 40);
    if (ret != ESP_OK) {
        esp3d_log_e("Failed to set threshold: %d", ret);
    }
    
    // Set active touch rate
    ret = touch_ft6336u_write_byte(FT6336U_TOUCHRATE_ACTIVE, 0x0E);
    if (ret != ESP_OK) {
        esp3d_log_e("Failed to set touch rate: %d", ret);
    }

    // Lire et afficher les informations de la puce une fois que tout est configuré
    uint8_t vendor_id = 0;
    uint8_t chip_id = 0;
    
    ret = touch_ft6336u_read_byte(FT6336U_VENDOR_ID, &vendor_id);
    if (ret != ESP_OK) {
        esp3d_log_e("Failed to read vendor ID: %d", ret);
        vendor_id = FT6336U_VENDID;  // Valeur par défaut
    }
    
    ret = touch_ft6336u_read_byte(FT6336U_CHIP_ID, &chip_id);
    if (ret != ESP_OK) {
        esp3d_log_e("Failed to read chip ID: %d", ret);
        chip_id = FT6336U_CHIPID;  // Valeur par défaut
    }
    
    esp3d_log("FT6336U detected with Vendor ID: 0x%02x, Chip ID: 0x%02x", vendor_id, chip_id);

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

    // Lecture de tous les registres en une seule transaction
    uint8_t touch_registers[16];  // Lire les 16 premiers registres 
    esp_err_t ret = touch_ft6336u_read_data(touch_registers, 16);
    
    if (ret != ESP_OK) {
        esp3d_log_e("Failed to read touch data: %d", ret);
        return data;
    }
    
    uint8_t touch_points = touch_registers[FT6336U_TOUCH_POINTS];
    
    if (touch_points > 0 && touch_points < 6) {
        data.is_pressed = true;
        
        // Extraire les coordonnées X et Y
        data.x = ((touch_registers[FT6336U_TOUCH1_XH] & 0x0F) << 8) | touch_registers[FT6336U_TOUCH1_XL];
        data.y = ((touch_registers[FT6336U_TOUCH1_YH] & 0x0F) << 8) | touch_registers[FT6336U_TOUCH1_YL];
        
        // Appliquer les transformations
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
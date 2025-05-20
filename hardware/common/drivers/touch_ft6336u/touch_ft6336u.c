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
#include "freertos/semphr.h"
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
static SemaphoreHandle_t touch_semaphore = NULL;

/* ISR handler for touch interrupt */
static void IRAM_ATTR touch_ft6336u_touch_isr_handler(void *arg) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    // Give semaphore from ISR
    if (touch_semaphore != NULL) {
        xSemaphoreGiveFromISR(touch_semaphore, &xHigherPriorityTaskWoken);
    }
    
    // Set flag for compatibility with existing code
    _touch_interrupt = true;
    
    // Yield if a higher priority task is woken
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

/* I2C communication functions with STOP/START sequence */
static esp_err_t touch_ft6336u_read_byte(uint8_t reg, uint8_t *data) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    
    // Send register address
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (_config.i2c_addr << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(_config.i2c_port, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    if (ret != ESP_OK) {
        return ret;
    }
    
    // Read data
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

/* Function to read a block of consecutive registers */
static esp_err_t touch_ft6336u_read_data(uint8_t *data, size_t len) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    
    // Position at register 0
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (_config.i2c_addr << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, 0, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(_config.i2c_port, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    if (ret != ESP_OK) {
        return ret;
    }
    
    // Read data
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

/* Simple ping test for the device */
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
esp_err_t touch_ft6336u_configure(const touch_ft6336u_config_t *config) {
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

    // Create interrupt semaphore
    if (touch_semaphore == NULL) {
        touch_semaphore = xSemaphoreCreateBinary();
        if (touch_semaphore == NULL) {
            esp3d_log_e("Failed to create touch semaphore");
            return ESP_ERR_NO_MEM;
        }
    }
    
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

    // Wait for I2C bus to stabilize
    vTaskDelay(pdMS_TO_TICKS(200));

    // Simple ping test to check if device responds
    ret = touch_ft6336u_ping();
    if (ret != ESP_OK) {
        esp3d_log_e("FT6336U ping failed: %d", ret);
        // Continue despite failure
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
        vTaskDelay(pdMS_TO_TICKS(20));  // Increased to 20ms
        gpio_set_level(config->rst_pin, 1);
        vTaskDelay(pdMS_TO_TICKS(300));  // Increased to 300ms to be safe
    }

    // Simplified initialization as in the FT6X36.cpp driver
    
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
    
    // Configure interrupt mode if needed
    if (GPIO_IS_VALID_GPIO(config->int_pin)) {
        ret = touch_ft6336u_write_byte(FT6336U_INTERRUPT_MODE, 1);  // 1 = Interrupt mode
        if (ret != ESP_OK) {
            esp3d_log_e("Failed to set interrupt mode: %d", ret);
        } else {
            esp3d_log("Touch controller configured for interrupt mode");
        }
    } else {
        ret = touch_ft6336u_write_byte(FT6336U_INTERRUPT_MODE, 0);  // 0 = Polling mode
        if (ret != ESP_OK) {
            esp3d_log_e("Failed to set polling mode: %d", ret);
        } else {
            esp3d_log("Touch controller configured for polling mode");
        }
    }

    // Read and display chip information after everything is configured
    uint8_t vendor_id = 0;
    uint8_t chip_id = 0;
    
    ret = touch_ft6336u_read_byte(FT6336U_VENDOR_ID, &vendor_id);
    if (ret != ESP_OK) {
        esp3d_log_e("Failed to read vendor ID: %d", ret);
        vendor_id = FT6336U_VENDID;  // Default value
    }
    
    ret = touch_ft6336u_read_byte(FT6336U_CHIP_ID, &chip_id);
    if (ret != ESP_OK) {
        esp3d_log_e("Failed to read chip ID: %d", ret);
        chip_id = FT6336U_CHIPID;  // Default value
    }
    
    esp3d_log("FT6336U detected with Vendor ID: 0x%02x, Chip ID: 0x%02x", vendor_id, chip_id);

    _is_initialized = true;
    esp3d_log("FT6336U initialized successfully");
    return ESP_OK;
}

void touch_ft6336u_deinit(void) {
    if (!_is_initialized) {
        return;
    }
    
    // Disable interrupt if configured
    if (GPIO_IS_VALID_GPIO(_config.int_pin)) {
        gpio_isr_handler_remove(_config.int_pin);
    }
    
    // Free the semaphore
    if (touch_semaphore != NULL) {
        vSemaphoreDelete(touch_semaphore);
        touch_semaphore = NULL;
    }
    
    // Uninstall I2C driver
    i2c_driver_delete(_config.i2c_port);
    
    _is_initialized = false;
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

    // If interrupt mode is enabled and INT pin is configured
    if (touch_semaphore != NULL && GPIO_IS_VALID_GPIO(_config.int_pin)) {
        // Wait for at most 10ms for an interrupt (non-blocking for LVGL)
        if (xSemaphoreTake(touch_semaphore, pdMS_TO_TICKS(10)) != pdTRUE) {
            // No interrupt in the timeout period, no touch
            return data;
        }
    } else {
        // In polling mode, directly check for touch
        if (!touch_ft6336u_detect_touch()) {
            return data;
        }
    }

    // Read all registers in a single transaction
    uint8_t touch_registers[16];  // Read first 16 registers
    esp_err_t ret = touch_ft6336u_read_data(touch_registers, 16);
    
    if (ret != ESP_OK) {
        esp3d_log_e("Failed to read touch data: %d", ret);
        return data;
    }
    
    uint8_t touch_points = touch_registers[FT6336U_TOUCH_POINTS];
    
    if (touch_points > 0 && touch_points < 6) {
        data.is_pressed = true;
        
        // Extract X and Y coordinates
        data.x = ((touch_registers[FT6336U_TOUCH1_XH] & 0x0F) << 8) | touch_registers[FT6336U_TOUCH1_XL];
        data.y = ((touch_registers[FT6336U_TOUCH1_YH] & 0x0F) << 8) | touch_registers[FT6336U_TOUCH1_YL];
        
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

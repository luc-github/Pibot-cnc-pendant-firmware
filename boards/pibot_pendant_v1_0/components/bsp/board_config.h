/*
  board_config.h - PiBot CNC Pendant hardware configuration

  Copyright (c) 2025 Luc LEBOSSE. All rights reserved.
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Board Identification */
#define BOARD_NAME      "PiBot CNC Pendant"
#define BOARD_VERSION   "v1.0"

/* Display Configuration */
// TFT Screen pin definitions
#define TFT_CS          GPIO_NUM_15
#define TFT_DC          GPIO_NUM_2
#define TFT_MOSI        GPIO_NUM_13
#define TFT_CLK         GPIO_NUM_14
#define TFT_MISO        GPIO_NUM_12
#define TFT_RST         -1  // Connected to ESP32 EN pin

// Display specifications
#define DISPLAY_WIDTH   240
#define DISPLAY_HEIGHT  320
#define DISPLAY_COLOR_BITS 16
#define DISPLAY_SWAP_COLOR 1

// Display SPI configuration
#define DISPLAY_SPI_HOST    SPI2_HOST
#define DISPLAY_SPI_FREQ    (40 * 1000 * 1000)  // 40 MHz
#define DISPLAY_SPI_MODE    0

// Display orientation flags
#define DISPLAY_SWAP_XY     0
#define DISPLAY_MIRROR_X    1
#define DISPLAY_MIRROR_Y    1
#define DISPLAY_INVERT      1

// Display buffer configuration
#define DISPLAY_USE_DOUBLE_BUFFER   1
#define DISPLAY_BUFFER_LINES        12  // 12 lines buffer

/* Touch Controller Configuration */
// Touch controller pin definitions
#define TOUCH_SDA        GPIO_NUM_32
#define TOUCH_SCL        GPIO_NUM_25
#define TOUCH_IRQ        GPIO_NUM_36
#define TOUCH_RST        -1  // No dedicated reset pin

// Touch controller I2C configuration
#define TOUCH_I2C_PORT   I2C_NUM_0
#define TOUCH_I2C_ADDR   0x38
#define TOUCH_I2C_FREQ   400000  // 400 KHz

// Touch controller orientation flags
#define TOUCH_SWAP_XY    0
#define TOUCH_MIRROR_X   0
#define TOUCH_MIRROR_Y   0

/* Backlight Configuration */
#define BACKLIGHT_PIN               GPIO_NUM_21
#define BACKLIGHT_ACTIVE_HIGH       1       // GPIO level for backlight on
#define BACKLIGHT_DEFAULT_LEVEL     100     // Default brightness level (0-100)
#define BACKLIGHT_PWM_ENABLED       0       // 0: GPIO control, 1: PWM control

// PWM specific settings (only used if BACKLIGHT_PWM_ENABLED is 1)
#define BACKLIGHT_PWM_FREQ          5000    // PWM frequency in Hz
#define BACKLIGHT_PWM_RESOLUTION    8       // Duty resolution
#define BACKLIGHT_PWM_TIMER         0       // Timer to use
#define BACKLIGHT_PWM_CHANNEL       0       // Channel to use

#ifdef __cplusplus
} /* extern "C" */
#endif

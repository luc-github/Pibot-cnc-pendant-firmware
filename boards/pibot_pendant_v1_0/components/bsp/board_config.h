/*
  board_config.h - PiBot CNC Pendant hardware configuration

  Copyright (c) 2025 Luc LEBOSSE. All rights reserved.
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif
// Helpers for stringification
// This is used to convert macros to string literals
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

/* Board Identification */
#define BOARD_NAME_STR      "PiBot CNC Pendant"
#define BOARD_VERSION_STR   "v1.0"

/* Display Configuration */
// TFT Screen pin definitions
#define DISPLAY_CS_PIN          GPIO_NUM_15
#define DISPLAY_DC_PIN          GPIO_NUM_2
#define DISPLAY_MOSI_PIN        GPIO_NUM_13
#define DISPLAY_CLK_PIN         GPIO_NUM_14
#define DISPLAY_MISO_PIN        GPIO_NUM_12
#define DISPLAY_RST_PIN         -1  // Connected to ESP32 EN pin

// Display specifications
#define DISPLAY_WIDTH_PX    240
#define DISPLAY_HEIGHT_PX   320
#define DISPLAY_COLOR_BITS_NB 16
#define DISPLAY_SWAP_COLOR_FLAG 1

// Display SPI configuration
#define DISPLAY_SPI_HOST_IDX    SPI3_HOST
#define DISPLAY_SPI_FREQ_HZ     (40 * 1000 * 1000)  // 40 MHz
#define DISPLAY_SPI_MODE_IDX    0

// Display orientation flags
#define DISPLAY_ORIENTATION      0
#define DISPLAY_INVERT_FLAG      1

// Display buffer configuration
#define DISPLAY_USE_DOUBLE_BUFFER_FLAG   1
#define DISPLAY_BUFFER_LINES_NB          12  // 12 lines buffer

/* Touch Controller Configuration */
// Touch controller pin definitions
#define TOUCH_SDA_PIN        GPIO_NUM_32
#define TOUCH_SCL_PIN        GPIO_NUM_25
#define TOUCH_IRQ_PIN        GPIO_NUM_36
#define TOUCH_RST_PIN        GPIO_NUM_NC  // No dedicated reset pin

// Touch controller I2C configuration
#define TOUCH_I2C_PORT_IDX   I2C_NUM_1
#define TOUCH_I2C_ADDR_HEX   0x38
#define TOUCH_I2C_FREQ_HZ    400000  // 400 KHz

// Touch controller orientation flags
#define TOUCH_SWAP_XY_FLAG    0
#define TOUCH_MIRROR_X_FLAG   0
#define TOUCH_MIRROR_Y_FLAG   0

/* Backlight Configuration */
#define BACKLIGHT_PIN               GPIO_NUM_21
#define BACKLIGHT_ACTIVE_HIGH_FLAG  1       // GPIO level for backlight on
#define BACKLIGHT_DEFAULT_LEVEL_PCT 100     // Default brightness level (0-100)
#define BACKLIGHT_PWM_ENABLED_FLAG  0       // 0: GPIO control, 1: PWM control

// PWM specific settings (only used if BACKLIGHT_PWM_ENABLED is 1)
#define BACKLIGHT_PWM_FREQ_HZ       5000    // PWM frequency in Hz
#define BACKLIGHT_PWM_RESOLUTION_BITS 8     // Duty resolution
#define BACKLIGHT_PWM_TIMER_IDX     0       // Timer to use
#define BACKLIGHT_PWM_CHANNEL_IDX   0       // Channel to use

/* SD Card Configuration */
#define SD_INTERFACE_TYPE   0  // 0: SPI, 1: SDIO
#define SD_MISO_PIN         GPIO_NUM_19
#define SD_MOSI_PIN         GPIO_NUM_23
#define SD_CLK_PIN          GPIO_NUM_18
#define SD_CS_PIN           GPIO_NUM_5

#define SD_DETECT_PIN       GPIO_NUM_NC   // Card detect pin
#define SD_DETECT_VALUE     0             // Card detect value (0 = LOW, 1 = HIGH)


// SD Card SPI configuration
#define SD_SPI_HOST_IDX     SPI2_HOST
#define SD_SPI_FREQ_HZ      (20 * 1000 * 1000)  // 20 MHz
#define SD_SPI_MODE_IDX     0
#define SD_SPI_SPEED_DIVIDER  1  // SPI speed divider (1 = 25 MHz, 2 = 12.5 MHz, etc.)
#define SD_MAX_TRANSFER_SIZE 4096
#define SD_ALLOCATION_SIZE   (4 * 1024)  // Allocation size for SD card


/* Button Matrix Configuration */
#define BUTTON_1_PIN        GPIO_NUM_4
#define BUTTON_2_PIN        GPIO_NUM_16
#define BUTTON_3_PIN        GPIO_NUM_17
#define BUTTON_DEBOUNCE_MS_TIME  20  // Debounce time in milliseconds

/* Rotary Encoder Configuration */
#define ENCODER_A_PIN       GPIO_NUM_22
#define ENCODER_B_PIN       GPIO_NUM_27
#define ENCODER_STEPS_PER_REV_NB  100  // Reference value
#define ENCODER_PULLUPS_ENABLED_FLAG 0  // No pull-up resistors installed
#define ENCODER_DEBOUNCE_US 1000           // 5ms debounce hardware
#define ENCODER_MIN_STEP_INTERVAL_US 10000 // 50ms anti-double-click

/* 4-Position Switch Configuration */
#define SWITCH_POS_1_PIN    GPIO_NUM_34
#define SWITCH_POS_2_PIN    GPIO_NUM_35
#define SWITCH_POS_3_PIN    GPIO_NUM_39
#define SWITCH_PULLUPS_ENABLED_FLAG 1  // 10K pull-up resistors installed
#define SWITCH_DEBOUNCE_MS_TIME  600  // Debounce time in milliseconds

/* Analog Input Configuration */
#define ANALOG_INPUT_PIN    GPIO_NUM_33
#define ANALOG_ADC_CHANNEL_IDX   ADC_CHANNEL_5  // Corresponding ADC channel for GPIO33
#define ANALOG_ADC_WIDTH_BITS    ADC_BITWIDTH_12
#define ANALOG_ADC_ATTEN_DB      ADC_ATTEN_DB_12
#define ANALOG_FILTER_ENABLED_FLAG 1
#define ANALOG_FILTER_SAMPLES_NB 5  // Number of samples for filtering

/* Buzzer Configuration */
#define BUZZER_PIN          GPIO_NUM_26
#define BUZZER_PWM_FREQ_HZ       2000  // Default buzzer frequency in Hz
#define BUZZER_PWM_RESOLUTION_BITS 8   // Duty resolution
#define BUZZER_PWM_TIMER_IDX      1    // Timer to use
#define BUZZER_PWM_CHANNEL_IDX    1    // Channel to use

/* UART Configuration */
#define UART_TX_PIN         GPIO_NUM_1
#define UART_RX_PIN         GPIO_NUM_3
#define UART_BAUD_RATE_BPS  115200
#define UART_BAUD_RATE_STR STR(UART_BAUD_RATE_BPS)
#define UART_PORT_IDX       UART_NUM_0
#define UART_DATA_BITS UART_DATA_8_BITS
#define UART_PARITY UART_PARITY_DISABLE
#define UART_STOP_BITS UART_STOP_BITS_1
#define UART_FLOW_CTRL UART_HW_FLOWCTRL_DISABLE
#define UART_SOURCE_CLK UART_SCLK_APB
#define UART_RX_BUFFER_SIZE 512
#define UART_TX_BUFFER_SIZE 0
#define UART_RX_FLUSH_TIMEOUT 1500  // milliseconds timeout
#define UART_RX_TASK_SIZE 4096
#define UART_TASK_CORE 1
#define UART_TASK_PRIORITY 10

/* LVGL Configuration */
#define LVGL_TICK_PERIOD_MS 10
#define LVGL_TASK_PRIORITY 3
#define LVGL_TASK_CORE 1
#define LVGL_TASK_STACK_SIZE 8192


#ifdef __cplusplus
} /* extern "C" */
#endif

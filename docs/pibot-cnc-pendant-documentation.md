# PiBot CNC Pendant Documentation

## Introduction

The PiBot CNC Pendant is an ESP32-based pendant designed to provide a comprehensive and versatile user interface. It integrates a color TFT display, touch controller, analog inputs, microSD card interface, buttons, rotary encoder, and a 4-position switch. This combination of interfaces enables the creation of various interactive projects, ranging from home automation controllers to robotic interface systems, monitoring and control systems.

## ESP32 GPIO Configuration

### GPIO Function Table (Numerical Order)

Below is a table listing all ESP32 GPIO pins from 0 to 39 and their functions in the PiBot:

| GPIO | Function | Component | Description |
|------|----------|-----------|-------------|
| GPIO0 | BOOT | System | Boot mode selection pin (strapping pin) |
| GPIO1 | TX0 | System UART0 | Serial interface transmit line |
| GPIO2 | TFT_DC | TFT Screen | Data/Command Select for display |
| GPIO3 | RX0 | System UART0 | Serial interface receive line |
| GPIO4 | Button 1| Button Matrix | Button with direct ground connection |
| GPIO5 | SD_CS | MicroSD Card | Chip Select for microSD |
| GPIO6 | SPI Flash | System | Internal SPI flash connection, not available for external use |
| GPIO7 | SPI Flash | System | Internal SPI flash connection, not available for external use |
| GPIO8 | SPI Flash | System | Internal SPI flash connection, not available for external use |
| GPIO9 | SPI Flash | System | Internal SPI flash connection, not available for external use |
| GPIO10 | SPI Flash | System | Internal SPI flash connection, not available for external use |
| GPIO11 | SPI Flash | System | Internal SPI flash connection, not available for external use |
| GPIO12 | TFT_MISO | TFT Screen | Master In Slave Out for display |
| GPIO13 | TFT_MOSI | TFT Screen | Master Out Slave In for display |
| GPIO14 | TFT_CLK | TFT Screen | SPI Clock for display |
| GPIO15 | TFT_CS | TFT Screen | Chip Select for display |
| GPIO16 | Button 2 | Button Matrix | Button with direct ground connection |
| GPIO17 | Button 3 | Button Matrix | Button with direct ground connection |
| GPIO18 | SD_CLK | MicroSD Card | Clock signal for microSD |
| GPIO19 | SD_MISO | MicroSD Card | Master In Slave Out for microSD |
| GPIO20 | Not Available | - | GPIO not exposed in ESP32 package |
| GPIO21 | TFT_LED | TFT Screen | Backlight Control |
| GPIO22 | Encoder A | Rotary Encoder | A signal (no pull-up resistors) |
| GPIO23 | SD_MOSI | MicroSD Card | Master Out Slave In for microSD |
| GPIO24 | Not Available | - | GPIO not exposed in ESP32 package |
| GPIO25 | TOUCH_SCL | Touch Controller | I2C Clock for FT6336U |
| GPIO26 | Buzzer (with SC8002B, passive) |  | |
| GPIO27 | Encoder B | Rotary Encoder | B signal (no pull-up resistors) |
| GPIO28 | Not Available | - | GPIO not exposed in ESP32 package |
| GPIO29 | Not Available | - | GPIO not exposed in ESP32 package |
| GPIO30 | Not Available | - | GPIO not exposed in ESP32 package |
| GPIO31 | Not Available | - | GPIO not exposed in ESP32 package |
| GPIO32 | TOUCH_SDA | Touch Controller | I2C Data for FT6336U |
| GPIO33 | Analog Input | Potentiometer | Center tap of 10K potentiometer  |
| GPIO34 | Switch Position 1 | 4-Position Switch | With 10K pull-up resistor |
| GPIO35 | Switch Position 2 | 4-Position Switch | With 10K pull-up resistor |
| GPIO36 | TOUCH_IRQ | Touch Controller | Interrupt signal from FT6336U |
| GPIO37 | Not Available | - | GPIO not exposed in ESP32 package |
| GPIO38 | Not Available | - | GPIO not exposed in ESP32 package |
| GPIO39 | Switch Position 3 | 4-Position Switch | With 10K pull-up resistor |

### Complete Pin Mapping

Below is a comprehensive table of all ESP32 physical pins and their functions in the PiBot:

| Pin | GPIO | Function | Component | Description |
|-----|------|----------|-----------|-------------|
| 1 | 3V3 | Power | Power Supply | 3.3V power output |
| 2 | EN | Reset | System | Reset/Enable pin (connected to TFT_RST) |
| 3 | GPIO36/VP | TOUCH_IRQ | Touch Controller | Interrupt signal from FT6336U |
| 4 | GPIO39/VN | Switch Position 3 | 4-Position Switch | With 10K pull-up resistor |
| 5 | GPIO34 | Switch Position 1 | 4-Position Switch | With 10K pull-up resistor |
| 6 | GPIO35 | Switch Position 2 | 4-Position Switch | With 10K pull-up resistor |
| 7 | GPIO32 | TOUCH_SDA | Touch Controller | I2C Data for FT6336U |
| 8 | GPIO33 | Analog Input | Potentiometer | Center tap of 10K potentiometer |
| 9 | GPIO25 | TOUCH_SCL | Touch Controller | I2C Clock for FT6336U |
| 10 | GPIO26 |Buzzer (with SC8002B, passive) |||
| 11 | GPIO27 | Encoder B | Rotary Encoder | B signal (no pull-up resistors) |
| 12 | GPIO14 | TFT_CLK | TFT Screen | SPI Clock for display |
| 13 | GPIO12 | TFT_MISO | TFT Screen | Master In Slave Out for display |
| 14 | GND | Ground | Power | Ground connection |
| 15 | GPIO13 | TFT_MOSI | TFT Screen | Master Out Slave In for display |
| 16 | GPIO9 | SPI Flash | System | Internal SPI flash connection, not available for external use |
| 17 | GPIO10 | SPI Flash | System | Internal SPI flash connection, not available for external use |
| 18 | GPIO11 | SPI Flash | System | Internal SPI flash connection, not available for external use |
| 19 | GPIO6 | SPI Flash | System | Internal SPI flash connection, not available for external use |
| 20 | GPIO7 | SPI Flash | System | Internal SPI flash connection, not available for external use |
| 21 | GPIO8 | SPI Flash | System | Internal SPI flash connection, not available for external use |
| 22 | GPIO5 | SD_CS | MicroSD Card | Chip Select for microSD |
| 23 | GPIO3/RX0 | RX0 | System UART0 | Serial interface receive line |
| 24 | GPIO1/TX0 | TX0 | System UART0 | Serial interface transmit line |
| 25 | GPIO0 | BOOT | System | Boot mode selection pin (strapping pin) |
| 26 | GPIO2 | TFT_DC | TFT Screen | Data/Command Select for display |
| 27 | GPIO4 | Button 1| Button Matrix | Button with direct ground connection |
| 28 | GPIO15 | TFT_CS | TFT Screen | Chip Select for display |
| 29 | GPIO16 | Button 2| Button Matrix | Button with direct ground connection |
| 30 | GPIO17 | Button 3| Button Matrix | Button with direct ground connection |
| 31 | GPIO18 | SD_CLK | MicroSD Card | Clock signal for microSD |
| 32 | GPIO19 | SD_MISO | MicroSD Card | Master In Slave Out for microSD |
| 33 | GPIO21 | TFT_LED | TFT Screen | Backlight Control |
| 34 | GPIO22 | Encoder A | Rotary Encoder | A signal (no pull-up resistors) |
| 35 | GPIO23 | SD_MOSI | MicroSD Card | Master Out Slave In for microSD |
| 36 | GND | Ground | Power | Ground connection |
| 37 | VIN | Power Input | Power | 5V power input |
| 38 | GPIO37 | Not Available | - | GPIO not exposed in ESP32 package |
| 39 | GPIO38 | Not Available | - | GPIO not exposed in ESP32 package |
| 40 | GPIO24 | Not Available | - | GPIO not exposed in ESP32 package |
| 41 | GPIO28 | Not Available | - | GPIO not exposed in ESP32 package |
| 42 | GPIO29 | Not Available | - | GPIO not exposed in ESP32 package |
| 43 | GPIO30 | Not Available | - | GPIO not exposed in ESP32 package |
| 44 | GPIO31 | Not Available | - | GPIO not exposed in ESP32 package |

## Detailed Specifications

### TFT Screen
- 3.2-inch TFT screen with 320×240 resolution
- ILI9341 LCD controller
- SPI interface
- Pin configuration:
  - TFT_CS (GPIO15): Chip Select
  - TFT_DC (GPIO2): Data/Command Select
  - TFT_MOSI (GPIO13): MOSI (SDI)
  - TFT_CLK (GPIO14): SCK Clock
  - TFT_MISO (GPIO12): MISO (SDO)
  - TFT_RST: Connected to ESP32 EN pin (no software control needed)
  - TFT_LED (GPIO21): Backlight Control

### Touch Controller
- FT6336U Touch Controller
- I2C interface
- Pin configuration:
  - TOUCH_SDA (GPIO32): I2C Data
  - TOUCH_SCL (GPIO25): I2C Clock
  - TOUCH_IRQ (GPIO36): Interrupt

### MicroSD Card Interface
- SPI interface
- Pin configuration:
  - MISO (GPIO19): Master In Slave Out
  - CS (GPIO5): Chip Select
  - MOSI (GPIO23): Master Out Slave In
  - CLK (GPIO18): Clock signal

### Button Matrix
- Three buttons with direct ground connection (no pull-up resistors)
- Pin configuration:
  - Button 1 (GPIO4)
  - Button 2 (GPIO16)
  - Button 3 (GPIO17)

### Rotary Encoder
- No pull-up resistors installed
- Pin configuration:
  - Encoder A (GPIO22)
  - Encoder B (GPIO27)

### 4-Position Switch
- Configured with 10K pull-up resistors
- Pin configuration:
  - Position 1 (GPIO34)
  - Position 2 (GPIO35)
  - Position 3 (GPIO39)
  - Position 0 (None active - all pins high due to pull-up resistors)

### Analog Input
- GPIO33 connected to center tap of 10K potentiometer (3.3V top, GND bottom)

### Buzzer
- Passive buzzer connected to GPIO26 with SC8002B audio amplifier

## Usage Notes
- The rotary encoder is configured without built-in pull-up resistors
- The button matrix buttons are connected directly to ground (no pull-up resistors)
- The TFT screen reset is tied to the ESP32 EN pin and does not require software control
- Internal SPI Flash pins (GPIO6-11) are not available for external use
- UART0 pins (GPIO1/TX0, GPIO3/RX0) are used for programming and debugging
- GPIOs 20, 24, 28-31, 37-38 are not physically exposed in the ESP32 package
- GPIO0 is a strapping pin that affects boot mode and should be carefully managed
- GPUs 34-39 are input-only pins and cannot be used as outputs

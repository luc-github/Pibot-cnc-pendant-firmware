/*
  disp_ili9341_spi.c

  Copyright (c) 2025 Luc Lebosse. All rights reserved.

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

#include "disp_ili9341_spi.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp3d_log.h"
#include "esp_err.h"
#include "esp_lcd_panel_commands.h"
#include "esp_lcd_panel_interface.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


// Version number
#define ESP_LCD_ILI9341_VER_MAJOR 1
#define ESP_LCD_ILI9341_VER_MINOR 0
#define ESP_LCD_ILI9341_VER_PATCH 0

// ILI9341 command definitions
#define LCD_CMD_NOP      0x00
#define LCD_CMD_SWRESET  0x01
#define LCD_CMD_SLPIN    0x10
#define LCD_CMD_SLPOUT   0x11
#define LCD_CMD_INVOFF   0x20
#define LCD_CMD_INVON    0x21
#define LCD_CMD_GAMMASET 0x26
#define LCD_CMD_DISPOFF  0x28
#define LCD_CMD_DISPON   0x29
#define LCD_CMD_CASET    0x2A
#define LCD_CMD_RASET    0x2B
#define LCD_CMD_RAMWR    0x2C
#define LCD_CMD_MADCTL   0x36
#define LCD_CMD_COLMOD   0x3A

// Static variables to store configuration and handles
static spi_ili9341_config_t spiIli9341Config;
static esp_lcd_panel_io_handle_t io_handle = NULL;
static esp_lcd_panel_handle_t panel_handle = NULL;
static bool _is_initialized                = false;

// Vendor-specific initialization commands for ILI9341
typedef struct
{
    uint8_t cmd;
    uint8_t data[16];
    uint8_t data_bytes;
    uint8_t delay_ms;
} ili9341_init_cmd_t;

#if ESP3D_TFT_LOG
const char *spi_names[] = {"SPI1_HOST", "SPI2_HOST", "SPI3_HOST"};
#endif  // ESP3D_TFT_LOG

// Default ILI9341 initialization sequence
static const ili9341_init_cmd_t ili9341_init_commands[] = {
    // Software Reset
    {0x01, {0}, 0, 50},  // 50 ms
    // Power Control A
    {0xCB, {0x39, 0x2C, 0x00, 0x34, 0x02}, 5, 0},
    // Power Control B
    {0xCF, {0x00, 0xC1, 0x30}, 3, 0},
    // Driver Timing Control A
    {0xE8, {0x85, 0x00, 0x78}, 3, 0},
    // Driver Timing Control B
    {0xEA, {0x00, 0x00}, 2, 0},
    // Power On Sequence Control
    {0xED, {0x64, 0x03, 0x12, 0x81}, 4, 0},
    // Pump Ratio Control
    {0xF7, {0x20}, 1, 0},
    // Power Control 1
    {0xC0, {0x23}, 1, 0},
    // Power Control 2
    {0xC1, {0x10}, 1, 0},
    // VCOM Control 1
    {0xC5, {0x3E, 0x28}, 2, 0},
    // VCOM Control 2
    {0xC7, {0x86}, 1, 0},
    // Memory Access Control
    {0x36, {0x48}, 1, 0},  // Orientation
    // Pixel Format Set (16 bits, RGB565)
    {0x3A, {0x55}, 1, 0},
    // Frame Rate Control (70 Hz)
    {0xB1, {0x00, 0x18}, 2, 0},
    // Display Function Control
    {0xB6, {0x08, 0x82, 0x27}, 3, 0},
    // 3Gamma Function Disable
    {0xF2, {0x00}, 1, 0},
    // Gamma Set
    {0x26, {0x01}, 1, 0},
    // Positive Gamma Correction
    {0xE0,
     {0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00},
     15,
     0},
    // Negative Gamma Correction
    {0xE1,
     {0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F},
     15,
     0},
    // Sleep Out
    {0x11, {0}, 0, 100},
    // Init with black
    {0x2A, {0x00, 0x00, 0x01, 0x3F}, 4, 0},  // resolution 320x240
    {0x2B, {0x00, 0x00, 0x00, 0xEF}, 4, 0},
    {0x2C, {0x00, 0x00}, 2, 20},  // Write black (RGB565: 0x0000)
    // Display On
    {0x29, {0}, 0, 100},  // 100 ms
    // Fin
    {0, {0}, 0xFF, 0}};

// Forward declarations for panel operations
static esp_err_t panel_ili9341_del(esp_lcd_panel_t *panel);
static esp_err_t panel_ili9341_reset(esp_lcd_panel_t *panel);
static esp_err_t panel_ili9341_init(esp_lcd_panel_t *panel);
static esp_err_t panel_ili9341_draw_bitmap(esp_lcd_panel_t *panel,
                                           int x_start,
                                           int y_start,
                                           int x_end,
                                           int y_end,
                                           const void *color_data);
static esp_err_t panel_ili9341_invert_color(esp_lcd_panel_t *panel, bool invert_color_data);
static esp_err_t panel_ili9341_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y);
static esp_err_t panel_ili9341_swap_xy(esp_lcd_panel_t *panel, bool swap_axes);
static esp_err_t panel_ili9341_set_gap(esp_lcd_panel_t *panel, int x_gap, int y_gap);
static esp_err_t panel_ili9341_disp_on_off(esp_lcd_panel_t *panel, bool off);

// ILI9341 panel structure
typedef struct
{
    esp_lcd_panel_t base;
    esp_lcd_panel_io_handle_t io;
    int reset_gpio_num;
    bool reset_level;
    int x_gap;
    int y_gap;
    uint8_t fb_bits_per_pixel;
    uint8_t madctl_val;  // save current value of LCD_CMD_MADCTL register
    uint8_t colmod_val;  // save current value of LCD_CMD_COLMOD register
    const ili9341_init_cmd_t *init_cmds;
    uint16_t init_cmds_size;
} ili9341_panel_t;

// Implementation of esp_lcd_new_panel_ili9341 - integrated directly in this file
static esp_err_t esp_lcd_new_panel_ili9341(const esp_lcd_panel_io_handle_t io,
                                           const esp_lcd_panel_dev_config_t *panel_dev_config,
                                           esp_lcd_panel_handle_t *ret_panel)
{
    esp_err_t ret            = ESP_OK;
    ili9341_panel_t *ili9341 = NULL;
    gpio_config_t io_conf    = {0};

    if (!io || !panel_dev_config || !ret_panel)
    {
        esp3d_log_e("Invalid argument for ILI9341 panel");
        return ESP_ERR_INVALID_ARG;
    }

    ili9341 = (ili9341_panel_t *)calloc(1, sizeof(ili9341_panel_t));
    if (!ili9341)
    {
        esp3d_log_e("No memory for ILI9341 panel");
        return ESP_ERR_NO_MEM;
    }

    if (panel_dev_config->reset_gpio_num >= 0)
    {
        io_conf.mode         = GPIO_MODE_OUTPUT;
        io_conf.pin_bit_mask = 1ULL << panel_dev_config->reset_gpio_num;
        ret                  = gpio_config(&io_conf);
        if (ret != ESP_OK)
        {
            esp3d_log_e("Configure GPIO for RST line failed");
            goto err;
        }
    }

    // Configure color format
    switch (panel_dev_config->rgb_ele_order)
    {
        case LCD_RGB_ELEMENT_ORDER_RGB:
            ili9341->madctl_val = 0;
            break;
        case LCD_RGB_ELEMENT_ORDER_BGR:
            ili9341->madctl_val |= LCD_CMD_BGR_BIT;
            break;
        default:
            esp3d_log_e("Unsupported RGB element order");
            ret = ESP_ERR_NOT_SUPPORTED;
            goto err;
    }

    // Configure bits per pixel
    switch (panel_dev_config->bits_per_pixel)
    {
        case 16:  // RGB565
            ili9341->colmod_val        = 0x55;
            ili9341->fb_bits_per_pixel = 16;
            break;
        case 18:  // RGB666
            ili9341->colmod_val = 0x66;
            // each color component (R/G/B) should occupy the 6 high bits of a byte, which means 3
            // full bytes are required for a pixel
            ili9341->fb_bits_per_pixel = 24;
            break;
        default:
            esp3d_log_e("Unsupported pixel width");
            ret = ESP_ERR_NOT_SUPPORTED;
            goto err;
    }

    ili9341->io             = io;
    ili9341->reset_gpio_num = panel_dev_config->reset_gpio_num;
    ili9341->reset_level    = panel_dev_config->flags.reset_active_high;

    // Setup panel function pointers
    ili9341->base.del          = panel_ili9341_del;
    ili9341->base.reset        = panel_ili9341_reset;
    ili9341->base.init         = panel_ili9341_init;
    ili9341->base.draw_bitmap  = panel_ili9341_draw_bitmap;
    ili9341->base.invert_color = panel_ili9341_invert_color;
    ili9341->base.set_gap      = panel_ili9341_set_gap;
    ili9341->base.mirror       = panel_ili9341_mirror;
    ili9341->base.swap_xy      = panel_ili9341_swap_xy;
    ili9341->base.disp_on_off  = panel_ili9341_disp_on_off;

    *ret_panel = &(ili9341->base);
    esp3d_log("New ILI9341 panel created");

    esp3d_log("LCD panel create success, version: %d.%d.%d",
              ESP_LCD_ILI9341_VER_MAJOR,
              ESP_LCD_ILI9341_VER_MINOR,
              ESP_LCD_ILI9341_VER_PATCH);

    return ESP_OK;

err:
    if (ili9341)
    {
        if (panel_dev_config->reset_gpio_num >= 0)
        {
            gpio_reset_pin(panel_dev_config->reset_gpio_num);
        }
        free(ili9341);
    }
    return ret;
}

static esp_err_t panel_ili9341_del(esp_lcd_panel_t *panel)
{
    ili9341_panel_t *ili9341 = __containerof(panel, ili9341_panel_t, base);

    if (ili9341->reset_gpio_num >= 0)
    {
        gpio_reset_pin(ili9341->reset_gpio_num);
    }
    esp3d_log("Deleting ILI9341 panel");
    free(ili9341);
    return ESP_OK;
}

static esp_err_t panel_ili9341_reset(esp_lcd_panel_t *panel)
{
    ili9341_panel_t *ili9341     = __containerof(panel, ili9341_panel_t, base);
    esp_lcd_panel_io_handle_t io = ili9341->io;

    // perform hardware reset
    if (ili9341->reset_gpio_num >= 0)
    {
        gpio_set_level(ili9341->reset_gpio_num, ili9341->reset_level);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level(ili9341->reset_gpio_num, !ili9341->reset_level);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    else
    {  // perform software reset
        esp_err_t ret = esp_lcd_panel_io_tx_param(io, LCD_CMD_SWRESET, NULL, 0);
        if (ret != ESP_OK)
        {
            esp3d_log_e("Send command failed");
            return ret;
        }
        vTaskDelay(pdMS_TO_TICKS(20));  // spec, wait at least 5ms before sending new command
    }

    return ESP_OK;
}

static esp_err_t panel_ili9341_init(esp_lcd_panel_t *panel)
{
    ili9341_panel_t *ili9341     = __containerof(panel, ili9341_panel_t, base);
    esp_lcd_panel_io_handle_t io = ili9341->io;
    esp_err_t ret;

    esp3d_log("Initializing ILI9341 panel");

    // LCD goes into sleep mode and display will be turned off after power on reset, exit sleep mode
    // first
    esp3d_log("Sending SLPOUT command to exit sleep mode");
    ret = esp_lcd_panel_io_tx_param(io, LCD_CMD_SLPOUT, NULL, 0);
    if (ret != ESP_OK)
    {
        esp3d_log_e("Send SLPOUT command failed: %s", esp_err_to_name(ret));
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(100));

    esp3d_log("Sending MADCTL command with value 0x%02x", ili9341->madctl_val);
    ret = esp_lcd_panel_io_tx_param(io,
                                    LCD_CMD_MADCTL,
                                    (uint8_t[]){
                                        ili9341->madctl_val,
                                    },
                                    1);
    if (ret != ESP_OK)
    {
        esp3d_log_e("Send MADCTL command failed: %s", esp_err_to_name(ret));
        return ret;
    }

    esp3d_log("Sending COLMOD command with value 0x%02x", ili9341->colmod_val);
    ret = esp_lcd_panel_io_tx_param(io,
                                    LCD_CMD_COLMOD,
                                    (uint8_t[]){
                                        ili9341->colmod_val,
                                    },
                                    1);
    if (ret != ESP_OK)
    {
        esp3d_log_e("Send COLMOD command failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Send the initialization commands
    esp3d_log("Sending initialization command sequence (%d commands)",
              sizeof(ili9341_init_commands) / sizeof(ili9341_init_cmd_t));

    for (int i = 0; i < sizeof(ili9341_init_commands) / sizeof(ili9341_init_cmd_t); i++)
    {
        // esp3d_log("Command %d: 0x%02X, %d bytes", i, ili9341_init_commands[i].cmd,
        // ili9341_init_commands[i].data_bytes);
        ret = esp_lcd_panel_io_tx_param(io,
                                        ili9341_init_commands[i].cmd,
                                        ili9341_init_commands[i].data,
                                        ili9341_init_commands[i].data_bytes);
        if (ret != ESP_OK)
        {
            esp3d_log_e("Send command 0x%02X failed: %s",
                        ili9341_init_commands[i].cmd,
                        esp_err_to_name(ret));
            return ret;
        }
        if (ili9341_init_commands[i].delay_ms > 0)
        {
            vTaskDelay(pdMS_TO_TICKS(ili9341_init_commands[i].delay_ms));
        }
    }

    esp3d_log("ILI9341 panel initialization completed successfully");
    return ESP_OK;
}

static esp_err_t panel_ili9341_draw_bitmap(esp_lcd_panel_t *panel,
                                           int x_start,
                                           int y_start,
                                           int x_end,
                                           int y_end,
                                           const void *color_data)
{
    ili9341_panel_t *ili9341 = __containerof(panel, ili9341_panel_t, base);
    assert((x_start < x_end) && (y_start < y_end)
           && "start position must be smaller than end position");
    esp_lcd_panel_io_handle_t io = ili9341->io;
    esp_err_t ret;

    x_start += ili9341->x_gap;
    x_end += ili9341->x_gap;
    y_start += ili9341->y_gap;
    y_end += ili9341->y_gap;

    // define an area of frame memory where MCU can access
    ret = esp_lcd_panel_io_tx_param(io,
                                    LCD_CMD_CASET,
                                    (uint8_t[]){
                                        (x_start >> 8) & 0xFF,
                                        x_start & 0xFF,
                                        ((x_end - 1) >> 8) & 0xFF,
                                        (x_end - 1) & 0xFF,
                                    },
                                    4);
    if (ret != ESP_OK)
    {
        esp3d_log_e("Send CASET command failed");
        return ret;
    }

    ret = esp_lcd_panel_io_tx_param(io,
                                    LCD_CMD_RASET,
                                    (uint8_t[]){
                                        (y_start >> 8) & 0xFF,
                                        y_start & 0xFF,
                                        ((y_end - 1) >> 8) & 0xFF,
                                        (y_end - 1) & 0xFF,
                                    },
                                    4);
    if (ret != ESP_OK)
    {
        esp3d_log_e("Send RASET command failed");
        return ret;
    }

    // transfer frame buffer
    size_t len = (x_end - x_start) * (y_end - y_start) * ili9341->fb_bits_per_pixel / 8;
    ret        = esp_lcd_panel_io_tx_color(io, LCD_CMD_RAMWR, color_data, len);
    if (ret != ESP_OK)
    {
        esp3d_log_e("Send color data failed");
        return ret;
    }

    return ESP_OK;
}

static esp_err_t panel_ili9341_invert_color(esp_lcd_panel_t *panel, bool invert_color_data)
{
    ili9341_panel_t *ili9341     = __containerof(panel, ili9341_panel_t, base);
    esp_lcd_panel_io_handle_t io = ili9341->io;
    int command                  = invert_color_data ? LCD_CMD_INVON : LCD_CMD_INVOFF;

    esp_err_t ret = esp_lcd_panel_io_tx_param(io, command, NULL, 0);
    if (ret != ESP_OK)
    {
        esp3d_log_e("Send invert color command failed");
        return ret;
    }

    return ESP_OK;
}

static esp_err_t panel_ili9341_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y)
{
    ili9341_panel_t *ili9341     = __containerof(panel, ili9341_panel_t, base);
    esp_lcd_panel_io_handle_t io = ili9341->io;

    if (mirror_x)
    {
        ili9341->madctl_val |= LCD_CMD_MX_BIT;
    }
    else
    {
        ili9341->madctl_val &= ~LCD_CMD_MX_BIT;
    }

    if (mirror_y)
    {
        ili9341->madctl_val |= LCD_CMD_MY_BIT;
    }
    else
    {
        ili9341->madctl_val &= ~LCD_CMD_MY_BIT;
    }

    esp_err_t ret =
        esp_lcd_panel_io_tx_param(io, LCD_CMD_MADCTL, (uint8_t[]){ili9341->madctl_val}, 1);
    if (ret != ESP_OK)
    {
        esp3d_log_e("Send MADCTL command failed");
        return ret;
    }

    return ESP_OK;
}

static esp_err_t panel_ili9341_swap_xy(esp_lcd_panel_t *panel, bool swap_axes)
{
    ili9341_panel_t *ili9341     = __containerof(panel, ili9341_panel_t, base);
    esp_lcd_panel_io_handle_t io = ili9341->io;

    if (swap_axes)
    {
        ili9341->madctl_val |= LCD_CMD_MV_BIT;
    }
    else
    {
        ili9341->madctl_val &= ~LCD_CMD_MV_BIT;
    }

    esp_err_t ret =
        esp_lcd_panel_io_tx_param(io, LCD_CMD_MADCTL, (uint8_t[]){ili9341->madctl_val}, 1);
    if (ret != ESP_OK)
    {
        esp3d_log_e("Send MADCTL command failed");
        return ret;
    }

    return ESP_OK;
}

static esp_err_t panel_ili9341_set_gap(esp_lcd_panel_t *panel, int x_gap, int y_gap)
{
    ili9341_panel_t *ili9341 = __containerof(panel, ili9341_panel_t, base);
    ili9341->x_gap           = x_gap;
    ili9341->y_gap           = y_gap;
    return ESP_OK;
}

static esp_err_t panel_ili9341_disp_on_off(esp_lcd_panel_t *panel, bool on_off)
{
    ili9341_panel_t *ili9341     = __containerof(panel, ili9341_panel_t, base);
    esp_lcd_panel_io_handle_t io = ili9341->io;
    int command                  = on_off ? LCD_CMD_DISPON : LCD_CMD_DISPOFF;

    esp_err_t ret = esp_lcd_panel_io_tx_param(io, command, NULL, 0);
    if (ret != ESP_OK)
    {
        esp3d_log_e("Send display on/off command failed");
        return ret;
    }

    return ESP_OK;
}

esp_err_t ili9341_spi_configure(const spi_ili9341_config_t *config)
{
    esp_err_t ret = ESP_OK;

    if (config == NULL)
    {
        esp3d_log_e("Invalid configuration");
        return ESP_ERR_INVALID_ARG;
    }

    memcpy(&spiIli9341Config, config, sizeof(spi_ili9341_config_t));

    // Enhanced logging
    esp3d_log("Configuring ILI9341 LCD Display over SPI");
    esp3d_log("SPI Configuration:");
    esp3d_log("    Host: %s", spi_names[spiIli9341Config.spi_bus.host]);
    esp3d_log("    MISO pin: %d", spiIli9341Config.spi_bus.miso_pin);
    esp3d_log("    MOSI pin: %d", spiIli9341Config.spi_bus.mosi_pin);
    esp3d_log("    SCLK pin: %d", spiIli9341Config.spi_bus.sclk_pin);
    esp3d_log("    CS pin: %d", spiIli9341Config.spi_bus.cs_pin);
    esp3d_log("    DC pin: %d", spiIli9341Config.spi_bus.dc_pin);
    esp3d_log("    RST pin: %d", spiIli9341Config.spi_bus.rst_pin);
    esp3d_log("    Clock speed: %ld Hz", spiIli9341Config.spi_bus.clock_speed_hz);
    esp3d_log("    Max transfer size: %d bytes", spiIli9341Config.spi_bus.max_transfer_sz);

    esp3d_log("Display Configuration:");
    esp3d_log("    Orientation: %d", spiIli9341Config.display.orientation);
    esp3d_log("    Invert colors: %s", spiIli9341Config.display.invert_colors ? "Yes" : "No");

    esp3d_log("Backlight Configuration:");
    esp3d_log("    Pin: %d", spiIli9341Config.backlight.pin);
    esp3d_log("    Active level: %s", spiIli9341Config.backlight.on_level ? "HIGH" : "LOW");

    esp3d_log("Interface Configuration:");
    esp3d_log("    Command bits: %d", spiIli9341Config.interface.cmd_bits);
    esp3d_log("    Parameter bits: %d", spiIli9341Config.interface.param_bits);
    esp3d_log("    Swap color bytes: %s",
              spiIli9341Config.interface.swap_color_bytes ? "Yes" : "No");

    esp3d_log("DMA Configuration:");
    esp3d_log("    Channel: %d", spiIli9341Config.dma.channel);
    esp3d_log("    QuadWP pin: %d", spiIli9341Config.dma.quadwp_pin);
    esp3d_log("    QuadHD pin: %d", spiIli9341Config.dma.quadhd_pin);

    // Configure backlight GPIO
    if (spiIli9341Config.backlight.pin >= 0)
    {
        gpio_config_t bk_gpio_config = {.mode         = GPIO_MODE_OUTPUT,
                                        .pin_bit_mask = 1ULL << spiIli9341Config.backlight.pin};
        if (spiIli9341Config.backlight.pin >= 0)
        {
            esp3d_log("Configuring backlight pin %d", spiIli9341Config.backlight.pin);
            ret = gpio_config(&bk_gpio_config);
            if (ret != ESP_OK)
            {
                esp3d_log_e("Failed to configure backlight GPIO: %s", esp_err_to_name(ret));
                return ret;
            }

            // Turn off backlight initially
            gpio_set_level(spiIli9341Config.backlight.pin, !spiIli9341Config.backlight.on_level);
            esp3d_log("Backlight initially turned off");
        }
    }
    else
    {
        esp3d_log("No backlight pin configured, skipping GPIO setup");
    }

    // Initialize SPI bus if this is the master
    if (spiIli9341Config.spi_bus.is_master)
    {
        esp3d_log("Initializing SPI bus as master");

        spi_bus_config_t buscfg = {
            .sclk_io_num     = spiIli9341Config.spi_bus.sclk_pin,
            .mosi_io_num     = spiIli9341Config.spi_bus.mosi_pin,
            .miso_io_num     = spiIli9341Config.spi_bus.miso_pin,
            .quadwp_io_num   = spiIli9341Config.dma.quadwp_pin,
            .quadhd_io_num   = spiIli9341Config.dma.quadhd_pin,
            .max_transfer_sz = spiIli9341Config.spi_bus.max_transfer_sz,
        };

        esp3d_log("SPI bus configuration:");
        esp3d_log("    SCLK: %d", buscfg.sclk_io_num);
        esp3d_log("    MOSI: %d", buscfg.mosi_io_num);
        esp3d_log("    MISO: %d", buscfg.miso_io_num);
        esp3d_log("    QuadWP: %d", buscfg.quadwp_io_num);
        esp3d_log("    QuadHD: %d", buscfg.quadhd_io_num);
        esp3d_log("    Max transfer size: %d bytes", buscfg.max_transfer_sz);

        ret = spi_bus_initialize(spiIli9341Config.spi_bus.host,
                                 &buscfg,
                                 spiIli9341Config.dma.channel);
        if (ret != ESP_OK)
        {
            esp3d_log_e("Failed to initialize SPI bus: %s", esp_err_to_name(ret));
            return ret;
        }
        esp3d_log("SPI bus initialized successfully on host %s",
                  spi_names[spiIli9341Config.spi_bus.host]);
    }
    else
    {
        esp3d_log("Using pre-initialized SPI bus");
    }

    // Configure LCD panel IO
    esp3d_log("Configuring LCD panel IO");
    esp_lcd_panel_io_spi_config_t io_config = {.dc_gpio_num = spiIli9341Config.spi_bus.dc_pin,
                                               .cs_gpio_num = spiIli9341Config.spi_bus.cs_pin,
                                               .pclk_hz = spiIli9341Config.spi_bus.clock_speed_hz,
                                               .lcd_cmd_bits = spiIli9341Config.interface.cmd_bits,
                                               .lcd_param_bits =
                                                   spiIli9341Config.interface.param_bits,
                                               .spi_mode          = 0,
                                               .trans_queue_depth = 10,
                                               .on_color_trans_done =
                                                   spiIli9341Config.lvgl.enable_callbacks
                                                       ? spiIli9341Config.lvgl.on_color_trans_done
                                                       : NULL,
                                               .user_ctx = spiIli9341Config.lvgl.enable_callbacks
                                                               ? spiIli9341Config.lvgl.user_ctx
                                                               : NULL};

    esp3d_log("LCD panel IO configuration:");
    esp3d_log("    DC GPIO: %d", io_config.dc_gpio_num);
    esp3d_log("    CS GPIO: %d", io_config.cs_gpio_num);
    esp3d_log("    Clock speed: %d Hz", io_config.pclk_hz);
    esp3d_log("    Command bits: %d", io_config.lcd_cmd_bits);
    esp3d_log("    Parameter bits: %d", io_config.lcd_param_bits);
    esp3d_log("    SPI mode: %d", io_config.spi_mode);
    esp3d_log("    Transaction queue depth: %d", io_config.trans_queue_depth);
    esp3d_log("    Color transfer callback: %s",
              spiIli9341Config.lvgl.enable_callbacks ? "Enabled" : "Disabled");

    ret = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)spiIli9341Config.spi_bus.host,
                                   &io_config,
                                   &io_handle);
    if (ret != ESP_OK)
    {
        esp3d_log_e("Failed to create LCD panel IO: %s", esp_err_to_name(ret));
        return ret;
    }
    esp3d_log("LCD panel IO created successfully");

    // Configure LCD panel
    esp3d_log("Creating ILI9341 panel driver");
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = spiIli9341Config.spi_bus.rst_pin,
        .rgb_ele_order  = spiIli9341Config.interface.swap_color_bytes ? LCD_RGB_ELEMENT_ORDER_BGR
                                                                      : LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,  // ILI9341 typically uses RGB565
    };

    esp3d_log("LCD panel configuration:");
    esp3d_log("    Reset GPIO: %d", panel_config.reset_gpio_num);
    esp3d_log("    RGB element order: %s",
              spiIli9341Config.interface.swap_color_bytes ? "BGR" : "RGB");
    esp3d_log("    Bits per pixel: %ld", panel_config.bits_per_pixel);

    ret = esp_lcd_new_panel_ili9341(io_handle, &panel_config, &panel_handle);
    if (ret != ESP_OK)
    {
        esp3d_log_e("Failed to create ILI9341 panel driver: %s", esp_err_to_name(ret));
        return ret;
    }
    esp3d_log("ILI9341 panel driver created successfully");

    // Reset and initialize panel
    esp3d_log("Resetting and initializing panel");
    ret = esp_lcd_panel_reset(panel_handle);
    if (ret != ESP_OK)
    {
        esp3d_log_e("Panel reset failed: %s", esp_err_to_name(ret));
        return ret;
    }
    esp3d_log("Panel reset successful");

    ret = esp_lcd_panel_init(panel_handle);
    if (ret != ESP_OK)
    {
        esp3d_log_e("Panel initialization failed: %s", esp_err_to_name(ret));
        return ret;
    }
    esp3d_log("Panel initialization successful");

    // Configure orientation based on the selected mode
    bool swap_xy  = false;
    bool mirror_x = false;
    bool mirror_y = false;

    switch (spiIli9341Config.display.orientation)
    {
        case SPI_ILI9341_ORIENTATION_PORTRAIT:
            swap_xy  = false;
            mirror_x = true;
            mirror_y = false;
            esp3d_log("Setting orientation to Portrait");
            break;
        case SPI_ILI9341_ORIENTATION_LANDSCAPE:
            swap_xy  = true;
            mirror_x = true;
            mirror_y = true;
            esp3d_log("Setting orientation to Landscape");
            break;
        case SPI_ILI9341_ORIENTATION_PORTRAIT_INVERTED:
            swap_xy  = false;
            mirror_x = false;
            mirror_y = true;
            esp3d_log("Setting orientation to Portrait Inverted");
            break;
        case SPI_ILI9341_ORIENTATION_LANDSCAPE_INVERTED:
            swap_xy  = true;
            mirror_x = false;
            mirror_y = false;
            esp3d_log("Setting orientation to Landscape Inverted");
            break;
    }

    esp3d_log("Panel orientation: swap_xy=%s, mirror_x=%s, mirror_y=%s",
              swap_xy ? "true" : "false",
              mirror_x ? "true" : "false",
              mirror_y ? "true" : "false");

    ret = esp_lcd_panel_swap_xy(panel_handle, swap_xy);
    if (ret != ESP_OK)
    {
        esp3d_log_e("Failed to set panel swap_xy: %s", esp_err_to_name(ret));
    }

    ret = esp_lcd_panel_mirror(panel_handle, mirror_x, mirror_y);
    if (ret != ESP_OK)
    {
        esp3d_log_e("Failed to set panel mirror: %s", esp_err_to_name(ret));
    }

    // Apply color inversion if needed
    if (spiIli9341Config.display.invert_colors)
    {
        esp3d_log("Inverting panel colors");
        ret = esp_lcd_panel_invert_color(panel_handle, true);
        if (ret != ESP_OK)
        {
            esp3d_log_e("Failed to invert panel colors: %s", esp_err_to_name(ret));
        }
    }

    // Turn on display
    esp3d_log("Turning on display");
    ret = esp_lcd_panel_disp_on_off(panel_handle, true);
    if (ret != ESP_OK)
    {
        esp3d_log_e("Failed to turn on display: %s", esp_err_to_name(ret));
        return ret;
    }

    // Turn on backlight
    if (spiIli9341Config.backlight.pin >= 0)
    {
        esp3d_log("Turning on backlight on pin %d with level %d",
                  spiIli9341Config.backlight.pin,
                  spiIli9341Config.backlight.on_level);
        // Backlight is contolled separately
        gpio_set_level(spiIli9341Config.backlight.pin, spiIli9341Config.backlight.on_level);
    }

    esp3d_log("ILI9341 display configured successfully");
    _is_initialized = true;
    return ESP_OK;
}

esp_lcd_panel_handle_t ili9341_spi_get_panel_handle(void)
{
    if (panel_handle == NULL)
    {
        esp3d_log_e("Panel not initialized, call ili9341_spi_configure first");
    }
    return panel_handle;
}

esp_lcd_panel_io_handle_t ili9341_spi_get_io_handle(void)
{
    if (io_handle == NULL)
    {
        esp3d_log_e("Panel IO not initialized, call ili9341_spi_configure first");
    }
    return io_handle;
}

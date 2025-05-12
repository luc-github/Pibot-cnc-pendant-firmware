# BSP (Board Support Package) component

# Board-specific sources
set(BSP_SRCS 
    "board_init.c"
)

# Include directories
set(INCLUDE_DIRS 
    "."
)

# Base requirements
set(BSP_REQUIRES
    "esp_timer"
    "esp3d_log"
    "nvs_flash"
)

# Check if TFT UI service is enabled
if(TFT_UI_SERVICE)
    list(APPEND BSP_REQUIRES
        "esp_lvgl_port"
        "lvgl"
        "esp_lcd"
        "display_ili9341_spi"
        "backlight"
    )
endif()

# Register the component with hardware-specific requirements
idf_component_register(
    SRCS ${BSP_SRCS}
    INCLUDE_DIRS ${INCLUDE_DIRS}
    REQUIRES ${BSP_REQUIRES}
)

message(STATUS "BSP component configuration for PiBot CNC Pendant:")
message(STATUS "Source files: ${BSP_SRCS}")
message(STATUS "TFT UI Service: ${TFT_UI_SERVICE}")
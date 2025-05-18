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
    "driver"
)

# Check if TFT UI service is enabled
if(TFT_UI_SERVICE)
    list(APPEND BSP_REQUIRES
        "lvgl"
        "esp_lcd"
        "ili9341"
        "backlight"
    )
endif()

# Register the component with hardware-specific requirements
idf_component_register(
    SRCS ${BSP_SRCS}
    INCLUDE_DIRS ${INCLUDE_DIRS}
    REQUIRES ${BSP_REQUIRES}
)

message(STATUS "\n${Cyan}BSP component configuration for PiBot CNC Pendant:${ColourReset}\n")
message(STATUS "${Cyan}Source files:${White} ${BSP_SRCS}${ColourReset}\n\n")

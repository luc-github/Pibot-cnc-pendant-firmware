# BSP (Board Support Package) component

# Board-specific sources
set(BSP_SRCS 
    "board_init.c"
)

# Include directories
set(INCLUDE_DIRS 
    "."
)

set(BSP_REQUIRES
    "esp_timer"
    "esp3d_log"
    "nvs_flash"
    "driver"
    "disp_backlight"
    "disp_ili9341"
    "touch_ft6336u"
    "lvgl"
)

message(STATUS "${Cyan}Adds Required:${White} ${BSP_REQUIRES}${ColourReset}\n\n")

# Register the component with hardware-specific requirements
idf_component_register(
    SRCS ${BSP_SRCS}
    INCLUDE_DIRS ${INCLUDE_DIRS}
    REQUIRES ${BSP_REQUIRES}
)

message(STATUS "\n${Cyan}BSP component configuration for PiBot CNC Pendant:${ColourReset}\n")
message(STATUS "${Cyan}Source files:${White} ${BSP_SRCS}${ColourReset}\n\n")

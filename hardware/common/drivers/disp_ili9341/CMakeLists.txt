# Component directive for ili9341 driver
# This component is responsible for controlling the display
set(SOURCES "disp_ili9341_spi.c")
set(INCLUDES .)
idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS ${INCLUDES}
    REQUIRES esp3d_log driver esp_lcd
)

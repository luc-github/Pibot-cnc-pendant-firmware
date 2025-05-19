# Component directive for backlight driver
# This component is responsible for controlling the backlight of the display
# Note:  the component name cannot be backlight because of some idf name collision, so use disp_backlight instead
set(SOURCES "disp_backlight.c")
set(INCLUDES .)
idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS ${INCLUDES}
    REQUIRES esp3d_log driver
)

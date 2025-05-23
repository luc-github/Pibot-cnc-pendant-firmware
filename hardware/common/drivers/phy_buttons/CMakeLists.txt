# Component directive for physical buttons driver
set(SOURCES "phy_buttons.c")
set(INCLUDES ".")
idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS ${INCLUDES}
    REQUIRES esp3d_log driver freertos esp_timer
)
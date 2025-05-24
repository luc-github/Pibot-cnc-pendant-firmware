# Component directive for 4-position switch driver
set(SOURCES "phy_switch.c")
set(INCLUDES ".")
idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS ${INCLUDES}
    REQUIRES esp3d_log driver freertos esp_timer
)

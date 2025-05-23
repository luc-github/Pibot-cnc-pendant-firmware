# Component directive for rotary encoder driver
set(SOURCES "phy_encoder.c")
set(INCLUDES ".")
idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS ${INCLUDES}
    REQUIRES esp3d_log driver freertos esp_timer
)
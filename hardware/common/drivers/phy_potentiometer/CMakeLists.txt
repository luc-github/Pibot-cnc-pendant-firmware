# Component directive for potentiometer driver
set(SOURCES "phy_potentiometer.c")
set(INCLUDES ".")
idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS ${INCLUDES}
    REQUIRES esp3d_log driver freertos esp_adc
)
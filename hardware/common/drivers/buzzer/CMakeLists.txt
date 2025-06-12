# Component directive for buzzer driver
# This component is responsible for controlling the buzzer
set(SOURCES "buzzer.c")
set(INCLUDES .)
idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS ${INCLUDES}
    REQUIRES esp3d_log driver esp_timer esp_common
)
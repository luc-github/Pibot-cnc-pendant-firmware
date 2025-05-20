# Component directive for FT6336U touch controller driver
set(SOURCES "touch_ft6336u.c")
set(INCLUDES .)
idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS ${INCLUDES}
    REQUIRES esp3d_log driver
)
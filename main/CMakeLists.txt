# main application directives

set(SOURCES .
    core core/commands
    modules/authentication
    modules/serial
    modules/network
    modules/filesystem
    modules/filesystem/flash
    modules/usb_serial
    modules/config_file
    modules/translations
    modules/gcode_host
)

if (BUZZER_SERVICE)
    list(APPEND SOURCES "modules/buzzer")
endif()

if(TIME_SERVICE)
    list(APPEND SOURCES "modules/time")
endif()

if(WIFI_SERVICE)
    if(SSDP_SERVICE)
        list(APPEND SOURCES "modules/ssdp")
    endif()

    if(MDNS_SERVICE)
        list(APPEND SOURCES "modules/mdns")
    endif()

    if(WEB_SERVICES)
        list(APPEND SOURCES "modules/http")
        list(APPEND SOURCES "modules/http/handlers")
        list(APPEND SOURCES "modules/http/handlers/authentication")
        list(APPEND SOURCES "modules/http/handlers/flash")
        if(SSDP_SERVICE)
            list(APPEND SOURCES "modules/http/handlers/ssdp")
        endif()
        if(SD_CARD_SERVICE)
            list(APPEND SOURCES "modules/http/handlers/sd")
        endif()
        if(UPDATE_SERVICE)
            list(APPEND SOURCES "modules/http/handlers/update")
        endif()
        if(WS_SERVICE)
            list(APPEND SOURCES "modules/http/handlers/ws")
        endif()
        if(WEBDAV_SERVICES)
            list(APPEND SOURCES "modules/http/handlers/webdav")
            list(APPEND SOURCES "modules/webdav")
        endif()
        if(CAMERA_SERVICE)
            list(APPEND SOURCES "modules/camera")
            list(APPEND SOURCES "modules/http/handlers/camera")
        endif()
        list(APPEND SOURCES "modules/websocket")
        set(WEBEMBEDDEDFILES "embedded/favicon.ico.gz" "embedded/index.html.gz")
    endif()

    if(TELNET_SERVICE)
        list(APPEND SOURCES "modules/socket_server")
    endif()

    if(NOTIFICATIONS_SERVICE)
        list(APPEND SOURCES "modules/notifications")
    endif()
endif()

if(BT_SERVICE)
    # TODO: add bluetooth support?
    list(APPEND SOURCES "modules/bt_serial")
    list(APPEND SOURCES "modules/bt_ble")
endif()

if(SD_CARD_SERVICE)
    list(APPEND SOURCES "modules/filesystem/sd")
endif()

if(UPDATE_SERVICE)
    list(APPEND SOURCES "modules/update")
endif()

 if(TFT_UI_SERVICE)
    list(APPEND SOURCES "display")
    list(APPEND SOURCES "display/components")
    list(APPEND SOURCES "display/screens")
    list(APPEND SOURCES "modules/rendering")
    if(TARGET_FW_MARLIN)
       list(APPEND SOURCES "display/3dprinter/marlin")
       list(APPEND SOURCES "display/3dprinter/marlin/screens")
    elseif(TARGET_FW_REPETIER)
       list(APPEND SOURCES "display/3dprinter/repetier")
       list(APPEND SOURCES "display/3dprinter/repetier/screens")
    elseif(TARGET_FW_SMOOTHIEWARE)
       list(APPEND SOURCES "display/3dprinter/smoothieware")
       list(APPEND SOURCES "display/3dprinter/smoothieware/screens")
    elseif(TARGET_FW_GRBL)
       list(APPEND SOURCES "display/cnc/grbl")
       list(APPEND SOURCES "display/cnc/grbl/screens")
    elseif(TARGET_FW_GRBLHAL)
       list(APPEND SOURCES "display/cnc/grblhal")
       list(APPEND SOURCES "display/cnc/grblhal/screens")
    elseif(TARGET_FW_FLUIDNC)
       list(APPEND SOURCES "display/cnc/fluidnc")
       list(APPEND SOURCES "display/cnc/fluidnc/screens")
    endif()
    if(TARGET_FW_MARLIN)
        list(APPEND SOURCES "display/3dprinter/marlin/${RESOLUTION_SCREEN}")
        set(INCLUDES_DISPLAY "display/3dprinter/marlin/${RESOLUTION_SCREEN}")
    elseif(TARGET_FW_REPETIER)
        list(APPEND SOURCES "display/3dprinter/repetier//${RESOLUTION_SCREEN}")
        set(INCLUDES_DISPLAY "display/3dprinter/repetier//${RESOLUTION_SCREEN}")
    elseif(TARGET_FW_SMOOTHIEWARE)
        list(APPEND SOURCES "display/3dprinter/smoothieware//${RESOLUTION_SCREEN}")
        set(INCLUDES_DISPLAY "display/3dprinter/smoothieware//${RESOLUTION_SCREEN}")
    elseif(TARGET_FW_GRBL)
        list(APPEND SOURCES "display/cnc/grbl//${RESOLUTION_SCREEN}")
        set(INCLUDES_DISPLAY "display/cnc/grbl//${RESOLUTION_SCREEN")
    elseif(TARGET_FW_GRBLHAL)
        list(APPEND SOURCES "display/cnc/grblhal//${RESOLUTION_SCREEN}")
        set(INCLUDES_DISPLAY "display/cnc/grblhal//${RESOLUTION_SCREEN}")
    elseif(TARGET_FW_FLUIDNC)
        list(APPEND SOURCES "display/cnc/fluidnc//${RESOLUTION_SCREEN}")
        set(INCLUDES_DISPLAY "display/cnc/fluidnc//${RESOLUTION_SCREEN}")
    endif()
endif()

if(TARGET_FW_MARLIN)
    list(APPEND SOURCES "target/3dprinter/marlin")
elseif(TARGET_FW_REPETIER)
   list(APPEND SOURCES "target/3dprinter/repetier")
elseif(TARGET_FW_SMOOTHIEWARE)
    list(APPEND SOURCES "target/3dprinter/smoothieware")
elseif(TARGET_FW_GRBL)
    list(APPEND SOURCES "target/cnc/grbl")
elseif(TARGET_FW_GRBLHAL)
    list(APPEND SOURCES "target/cnc/grblhal")
elseif(TARGET_FW_FLUIDNC)
    list(APPEND SOURCES "target/cnc/fluidnc")
endif()

set(INCLUDES .
    ${INCLUDES_DISPLAY}
    core/includes
    modules)
idf_component_register(SRC_DIRS ${SOURCES}
    INCLUDE_DIRS ${INCLUDES}
    EMBED_FILES ${WEBEMBEDDEDFILES}  ${UIEMBEDDEDFILES})


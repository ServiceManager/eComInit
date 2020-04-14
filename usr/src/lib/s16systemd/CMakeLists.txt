project (s16systemd)

set(SD_ROOT "${CMAKE_SOURCE_DIR}/vendor/SystemD/src")

add_library (s16systemd SHARED ${SD_ROOT}/libsystemd-daemon/sd-daemon.c)

if(S16_ENABLE_SD_NOTIFY)
    target_compile_definitions(s16systemd PRIVATE S16_ENABLE_SD_NOTIFY)
endif()

if(LINUX)
    target_link_libraries(s16systemd PUBLIC rt)
endif()

target_include_directories(s16systemd PUBLIC
    $<BUILD_INTERFACE:${SD_ROOT}>
    $<BUILD_INTERFACE:${SD_ROOT}/systemd>
    )

set_target_properties(s16systemd PROPERTIES
    PUBLIC_HEADER "${SD_ROOT}/systemd/sd-daemon.h"
    )

install(TARGETS s16systemd DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/s16systemd
    )
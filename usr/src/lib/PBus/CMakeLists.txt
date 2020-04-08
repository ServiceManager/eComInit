project (LibPBus)

add_library (PBus_priv INTERFACE)
target_include_directories(PBus_priv INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_library (PBus SHARED PBus.c)
target_link_libraries (PBus s16 s16systemd)

set(PBUS_HEADERS
    hdr/PBus.h
  )

set_target_properties(PBus PROPERTIES
    PUBLIC_HEADER "${PBUS_HEADERS}"
    )

install(TARGETS PBus DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION include
        )
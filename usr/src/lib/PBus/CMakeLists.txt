project (LibPBus)

set(hdrSub PBus)
set(hdrPath ${PROJECT_SOURCE_DIR}/hdr/${hdrSub})

add_library (PBus_priv INTERFACE)
target_include_directories(PBus_priv INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_library (PBus SHARED PBusObject.c PBusConnection.c)
target_link_libraries (PBus s16 s16systemd)
target_include_directories(PBus PUBLIC ${PROJECT_SOURCE_DIR}/hdr)

set(PBUS_HEADERS
    ${hdrPath}/PBus.h
  )

set_target_properties(PBus PROPERTIES
    PUBLIC_HEADER "${PBUS_HEADERS}"
    )

install(TARGETS PBus DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${hdrSub}/
        )

if (S16_ENABLE_TESTS)
  addTest(pbus PBus)

  addTests(${s16_test_list})
endif()

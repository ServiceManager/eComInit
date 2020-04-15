project (PBus-Monitor)

add_executable (PBus-Monitor PBus-Monitor.c)
target_link_libraries (PBus-Monitor s16 PBus)

install(TARGETS PBus-Monitor RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

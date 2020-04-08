project (PBusBroker)

add_executable (PBusBroker PBusBroker.c)
target_link_libraries (PBusBroker s16 PBus PBus_priv)

install(TARGETS PBusBroker RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR})

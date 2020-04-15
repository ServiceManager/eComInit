project (PBus-Broker)

add_executable (PBus-Broker PBus-Broker.c)
target_link_libraries (PBus-Broker s16 PBus PBus_priv)

install(TARGETS PBus-Broker RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR})

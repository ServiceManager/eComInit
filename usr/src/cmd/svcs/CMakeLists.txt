cmake_minimum_required (VERSION 2.8)
project (svcs)

add_executable (svcs svcs.c)

target_link_libraries (svcs s16 ucl)

install(TARGETS svcs RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

cmake_minimum_required (VERSION 2.8)
project (s16systemd-notify)

add_executable (s16systemd-notify notify.c)

target_link_libraries (s16systemd-notify s16systemd uthash)

install(TARGETS s16systemd-notify RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

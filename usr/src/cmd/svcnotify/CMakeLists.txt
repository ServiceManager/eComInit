cmake_minimum_required (VERSION 2.8)
project (svcnotify)

add_executable (svcnotify notify.c)

target_link_libraries (svcnotify s16systemd uthash)

install(TARGETS svcnotify RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR})

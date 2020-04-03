cmake_minimum_required (VERSION 2.8)
project (s16db)

add_library (s16db SHARED rpc.c local.c convert.c)
target_link_libraries (s16db ucl s16rpc Kqueue::Kqueue)

install(TARGETS s16db DESTINATION ${CMAKE_INSTALL_LIBDIR})
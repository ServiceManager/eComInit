cmake_minimum_required (VERSION 2.8)
project (s16rpc)

add_library (s16rpc SHARED rpc.c)
target_link_libraries (s16rpc ucl ${LIBKQUEUE_LIBRARY})

install(TARGETS s16rpc DESTINATION ${CMAKE_INSTALL_LIBDIR})
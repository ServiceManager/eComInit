cmake_minimum_required (VERSION 2.8)
project (s16.configd)

add_executable (s16.configd configd.c db.c rpc.c)
target_link_libraries (s16.configd s16 s16db s16rpc ucl ${LIBKQUEUE_LIBRARY})

install(TARGETS s16.configd RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR})

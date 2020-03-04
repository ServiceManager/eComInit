cmake_minimum_required (VERSION 2.8)
project (svcs)

add_executable (svcs svcs.c)

target_link_libraries (svcs s16 s16db s16rpc ucl ${LIBKQUEUE_LIBRARY})

install(TARGETS svcs RUNTIME DESTINATION bin)

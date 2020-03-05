cmake_minimum_required (VERSION 2.8)
project (s16.graphd)

add_executable (s16.graphd graphd.c graph.c)
target_link_libraries (s16.graphd s16 s16db s16rr uthash ${LIBKQUEUE_LIBRARY})

install(TARGETS s16.graphd RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR})

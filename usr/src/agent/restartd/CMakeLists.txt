cmake_minimum_required (VERSION 2.8)
project (s16.registryd)

add_executable (s16.restartd restartd.c timer.c unit.c)
target_link_libraries (s16.restartd s16 s16db s16rr ${LIBKQUEUE_LIBRARY})

install(TARGETS s16.restartd RUNTIME DESTINATION lib/s16/bin)

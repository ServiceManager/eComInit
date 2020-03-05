cmake_minimum_required (VERSION 2.8)
project (s16)

configure_file (${CMAKE_SOURCE_DIR}/hdr/s16compat-tpl.h ${PROJECT_BINARY_DIR}/s16compat.h)

add_library (s16 SHARED mem.c misc.c s16.c)

if(LIBSTDTHREADS)
  message("Using Libstdthreads")
  target_link_libraries (s16 stdthreads)
else()
  target_link_libraries (s16 pthread)
endif()

target_include_directories(s16 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)

set_target_properties(s16 PROPERTIES
    PUBLIC_HEADER "${PROJECT_BINARY_DIR}/s16compat.h;${CMAKE_SOURCE_DIR}/hdr/s16.h"
    )

install(TARGETS s16 DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION include
        )
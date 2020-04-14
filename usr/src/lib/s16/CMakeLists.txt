project (s16)

set(hdrSub "S16")
set(hdrPath ${PROJECT_SOURCE_DIR}/hdr/${hdrSub})

configure_file (${hdrPath}/PlatformDefinitions-tpl.h ${PROJECT_BINARY_DIR}/S16/PlatformDefinitions.h)

if (FREEBSD OR NETBSD OR OPENBSD OR DRAGONFLY)
  set (PT_DRIVER "kevent")
elseif (LINUX)
  set (PT_DRIVER "netlink")
elseif (POSIX)
  set (PT_DRIVER "posix")
endif()

add_library (s16 SHARED 
  mem.c misc.c s16.c 
  rpc/rpc.c
  newrpc/clnt.c newrpc/struct.c
  db/convert.c db/local.c db/rpc.c
  rr/process.c rr/process-tracker/pt-driver-${PT_DRIVER}.c
)

if(LIBSTDTHREADS)
  message("Using Libstdthreads")
  target_link_libraries (s16 stdthreads)
else()
  target_link_libraries (s16 pthread)
endif()

target_link_libraries (s16 ucl uthash nvp Kqueue::Kqueue m)
target_include_directories(s16 PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/hdr>
)

set(S16_HEADERS
  ${PROJECT_BINARY_DIR}/${hdrSub}/PlatformDefinitions.h
  ${hdrPath}/Core.h
  ${hdrPath}/List.h
  ${hdrPath}/Service.h
  ${hdrPath}/Repository.h
)

set_target_properties(s16 PROPERTIES
    PUBLIC_HEADER "${S16_HEADERS}"
)

install(TARGETS s16 DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${hdrSub}
)

if (S16_ENABLE_TESTS)
  addTest(newrpc s16)
  addTest(db s16)

  addTests(${s16_test_list})
endif()

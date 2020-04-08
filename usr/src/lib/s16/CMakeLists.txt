project (s16)

configure_file (${CMAKE_SOURCE_DIR}/hdr/s16compat-tpl.h ${PROJECT_BINARY_DIR}/s16compat.h)

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
  rr/process.c rr/process-tracker/pt-driver-${PT_DRIVER}.c)

if(LIBSTDTHREADS)
  message("Using Libstdthreads")
  target_link_libraries (s16 stdthreads)
else()
  target_link_libraries (s16 pthread)
endif()

target_link_libraries (s16 ucl uthash nvp Kqueue::Kqueue)
target_include_directories(s16 PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)

set(S16_HEADERS
  ${PROJECT_BINARY_DIR}/s16compat.h
  ${CMAKE_SOURCE_DIR}/hdr/s16.h
  ${CMAKE_SOURCE_DIR}/hdr/s16list.h
  ${CMAKE_SOURCE_DIR}/hdr/s16db.h
  ${CMAKE_SOURCE_DIR}/hdr/s16db_priv.h
  )

set_target_properties(s16 PROPERTIES
    PUBLIC_HEADER "${S16_HEADERS}"
    )

install(TARGETS s16 DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION include
        )


#add_executable (tester newrpc/struct.c)
#target_link_libraries (tester s16)
#
# Tests
#
if (S16_ENABLE_TESTS)

  function(addTest name)
    message("S16 Add test ${name}")
    add_executable (${name} tests/${name}.c)
    target_include_directories(${name} PRIVATE ${PROJECT_SOURCE_DIR})
    target_link_libraries (${name} s16 Atf-C::Atf-C)
    set(s16_test_list ${s16_test_list} ${name} PARENT_SCOPE)
  endfunction()

  addTest(newrpc)
  addTest(db)

  addTests(${s16_test_list})
endif()
include (CheckIncludeFiles)
include (CheckLibraryExists)
include (FindPkgConfig)
include (cmake/LemFlex.cmake)
include(GNUInstallDirs)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

cmake_minimum_required(VERSION 3.1)
project("System XVI")

# add flags used in the entire project
set(WARN_FLAGS "-Wall -Wno-unused-function -Wno-switch")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARN_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARN_FLAGS}")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(FLEX)
find_package(Readline)

check_include_files(threads.h C11_THREADS)

if(NOT C11_THREADS)
	message("Using internal implementation of C11 threads")
	include_directories("${PROJECT_SOURCE_DIR}/vendor/c11thrd")
endif()

check_library_exists(stdthreads "mtx_lock" "" LIBSTDTHREADS)

find_package(Libkqueue REQUIRED)

if(UNIX AND NOT APPLE)
  if(CMAKE_SYSTEM_NAME MATCHES ".*Linux")
    set(LINUX TRUE)
    add_definitions("-D_GNU_SOURCE")
  elseif(CMAKE_SYSTEM_NAME MATCHES "kFreeBSD.*|FreeBSD")
    set(S16_PLAT_BSD TRUE)
    set(FREEBSD TRUE)
  elseif(CMAKE_SYSTEM_NAME MATCHES "kNetBSD.*|NetBSD.*")
    set(S16_PLAT_BSD TRUE)
    set(NETBSD TRUE)
  elseif(CMAKE_SYSTEM_NAME MATCHES "kOpenBSD.*|OpenBSD.*")
    set(S16_PLAT_BSD TRUE)
    set(OPENBSD TRUE)
    # no credential passing over Unix datagram sockets in OpenBSD
    set(S16_ENABLE_SD_NOTIFY FALSE)
  elseif(CMAKE_SYSTEM_NAME MATCHES "DragonFly.*")
    set(S16_PLAT_BSD TRUE)
    set(DRAGONFLY TRUE)
  endif()
elseif(APPLE)
  set(S16_PLAT_BSD TRUE)
  # no credential passing over Unix datagram sockets in OS X?
  set(S16_ENABLE_SD_NOTIFY FALSE)
  set(OSX TRUE)
endif()

# java related
find_package(Java COMPONENTS Development)
find_package(JNI)

include_directories("${PROJECT_SOURCE_DIR}/hdr")
include_directories("${CMAKE_SOURCE_DIR}")
#include_directories(${Readline_INCLUDE_DIR})

set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib)

if(NOT Java_Development_FOUND)
    message("JDK not found; will not build Java components")
    set(S16_ENABLE_JAVA FALSE)
elseif (NOT JNI_FOUND)
    message("JNI not found; will not build Java components")
    set(S16_ENABLE_JAVA FALSE)
else()
    message("JDK and JNI found; target `java` builds Java components.")
    include(UseJava)
    add_subdirectory(java)
endif()

function(FIfUnset flag enable)
    if(NOT DEFINED ${flag})
        set(${flag} ${enable} PARENT_SCOPE)
    endif()
endfunction(FIfUnset)

FIfUnset(S16_ENABLE_JAVA TRUE)
FIfUnset(S16_ENABLE_SD_NOTIFY TRUE)

FIfUnset(S16_ENABLE_TESTS TRUE)
if(S16_ENABLE_TESTS)
  enable_testing()
  add_subdirectory(../../external/googletest googletest)
  add_subdirectory(../tests tests)
endif()

add_subdirectory(etc)

add_subdirectory(vendor/lemon)

add_subdirectory(lib/ucl)
add_subdirectory(lib/uthash)

add_subdirectory(lib/s16)
add_subdirectory(lib/s16rpc)
add_subdirectory(lib/s16db)
add_subdirectory(lib/s16rr)
add_subdirectory(lib/s16systemd)

add_subdirectory(agent/restartd)
add_subdirectory(agent/configd)
add_subdirectory(agent/graphd)

add_subdirectory(cmd/svcadm)
add_subdirectory(cmd/svccfg)
add_subdirectory(cmd/svcnotify)
add_subdirectory(cmd/svcs)

#add_subdirectory(test/cxx-build)
#add_custom_target(test DEPENDS test-cxx-build)

function(FShow name flag)
    if(${flag})
        message("  ${name}: Enabled")
    else()
        message("  ${name}: Disabled")
    endif()
endfunction(FShow)

message("Feature settings:")
FShow("Java" S16_ENABLE_JAVA)
FShow("SystemD Notification Interface" S16_ENABLE_SD_NOTIFY)
FShow("Tests" S16_ENABLE_TESTS)

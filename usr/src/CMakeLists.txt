include (CheckIncludeFiles)
include (CheckLibraryExists)
include (FindPkgConfig)
include (cmake/LemFlex.cmake)
include(GNUInstallDirs)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

cmake_minimum_required(VERSION 2.8.0)
project("System XVI")

set(WARN_FLAGS "-Wall -Wno-unused-function -Wno-switch")
# add flags used in the entire project
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_GNU_SOURCE -std=c11 -fPIC ${WARN_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GNU_SOURCE -std=c++11 -fPIC ${WARN_FLAGS}")

find_package(FLEX)
find_package(Readline)

CHECK_INCLUDE_FILES (threads.h C11_THREADS)

if(NOT C11_THREADS)
	message("Using internal implementation of C11 threads")
	include_directories("${PROJECT_SOURCE_DIR}/vendor/c11thrd")
endif()

CHECK_LIBRARY_EXISTS(stdthreads "mtx_lock" "" LIBSTDTHREADS)

if(UNIX AND NOT APPLE)
  if(CMAKE_SYSTEM_NAME MATCHES ".*Linux")
    set(LINUX TRUE)
    find_package(Libkqueue REQUIRED)
    if(NOT LIBKQUEUE_FOUND)
      message(FATAL_ERROR "Linux libkqueue was not found." )
    else()
      include_directories(${LIBKQUEUE_INCLUDE_DIR})
    endif()
  elseif(CMAKE_SYSTEM_NAME MATCHES "kFreeBSD.*|FreeBSD")
    set(S16_PLAT_BSD TRUE)
    set(FREEBSD TRUE)
  elseif(CMAKE_SYSTEM_NAME MATCHES "kNetBSD.*|NetBSD.*")
    set(S16_PLAT_BSD TRUE)
    set(NETBSD TRUE)
  elseif(CMAKE_SYSTEM_NAME MATCHES "kOpenBSD.*|OpenBSD.*")
    set(S16_PLAT_BSD TRUE)
    set(OPENBSD TRUE)
  elseif(CMAKE_SYSTEM_NAME MATCHES "DragonFly.*")
    set(S16_PLAT_BSD TRUE)
    set(DRAGONFLY TRUE)
  endif()
elseif(APPLE)
  set(S16_PLAT_BSD TRUE)
  set(OSX TRUE)
endif()

# java related
find_package(Java COMPONENTS Development)
find_package(JNI)

include_directories("${PROJECT_SOURCE_DIR}/hdr")
include_directories("${CMAKE_SOURCE_DIR}")
include_directories(${Readline_INCLUDE_DIR})

SET(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib)

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
add_subdirectory(cmd/svcs)
add_subdirectory(cmd/s16systemd-notify)

if(NOT Java_Development_FOUND)
    message("JDK not found; will not build Java components")
elseif (NOT JNI_FOUND)
    message("JNI not found; will not build Java components")
else()
    message("JDK and JNI found; target `java` builds Java components.")
    include(UseJava)
    add_subdirectory(java)
endif()

add_subdirectory(test/cxx-build)
add_custom_target(test DEPENDS test-cxx-build)

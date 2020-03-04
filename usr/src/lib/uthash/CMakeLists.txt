cmake_minimum_required (VERSION 2.8)
project (uthash)

add_library (uthash INTERFACE)
target_include_directories(uthash INTERFACE ${CMAKE_SOURCE_DIR}/../../external/uthash/src)

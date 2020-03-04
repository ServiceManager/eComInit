cmake_minimum_required (VERSION 2.8)
project (s16_jni)

add_library (s16_jni SHARED EXCLUDE_FROM_ALL s16_jni.c)
target_link_libraries (s16_jni s16 s16db s16rpc ucl ${LIBKQUEUE_LIBRARY})

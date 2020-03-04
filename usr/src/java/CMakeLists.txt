add_subdirectory(lib/s16_jni)
add_subdirectory(lib/s16j)
add_custom_target(java DEPENDS s16_jni s16j)

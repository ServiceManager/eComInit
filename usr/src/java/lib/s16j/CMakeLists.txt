cmake_minimum_required (VERSION 2.8)
project (s16j)

set(libs16_PFX "src/org/systemxvi/libs16")
SET(JAVA_SOURCE_FILES ${libs16_PFX}/Path.java)
    
add_jar (s16j ${JAVA_SOURCE_FILES})
set_target_properties(s16j PROPERTIES EXCLUDE_FROM_ALL TRUE)
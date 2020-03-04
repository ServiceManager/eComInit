cmake_minimum_required (VERSION 2.8)
project (test-cxx-build)

add_executable (test-cxx-build EXCLUDE_FROM_ALL test-cxx-build.cpp)
target_link_libraries (test-cxx-build s16 s16db s16rr uthash ${LIBKQUEUE_LIBRARY})

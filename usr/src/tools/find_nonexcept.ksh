#!/usr/bin/env ksh93

srcdir=$1
builddir=$2
taskname=$3

find "$1" "$2" \
       -name '*.c' -o -name '*.h' \
    -o -name '*.cc' -o -name '*.cpp' \
    -o -name '*.m' \
  | grep -v  -e vendor -e CMakeFiles \
  | grep -v -f "$1"/../../exception_lists/$3

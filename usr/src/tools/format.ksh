find usr/src usr/tests -name '*.c' -o -name '*.h' | grep -v vendor | xargs clang-format -i

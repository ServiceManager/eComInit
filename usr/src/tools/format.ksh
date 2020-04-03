find usr/src usr/tests -name '*.c' -o -name '*.h' | grep -v libucl | grep -v uthash | xargs clang-format -i

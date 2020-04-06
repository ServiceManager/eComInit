find usr/src usr/tests -name '*.c' -o -name '*.h' | grep -v libucl | grep -v uthash | grep -v vendor/freebsd | xargs clang-format -i

cmake_minimum_required (VERSION 2.4)
project (lemon)

add_executable (lemon lemon.c)
add_executable (lemonxx lemon.c)
target_compile_definitions (lemonxx PUBLIC LEMONPLUSPLUS)
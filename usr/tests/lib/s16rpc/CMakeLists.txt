add_executable (s16rpc_tests tests.cpp)
target_link_libraries (s16rpc_tests s16rpc gtest_main)

add_test(NAME s16rpc_tests COMMAND s16rpc_tests)
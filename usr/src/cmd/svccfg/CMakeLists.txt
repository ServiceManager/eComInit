cmake_minimum_required (VERSION 2.8)
project (svccfg)

FlexComp(svccfg.l)
LemonComp(svccfg.y)

add_executable (svccfg
    ${PROJECT_BINARY_DIR}/svccfg.tab.c ${PROJECT_BINARY_DIR}/svccfg.y.c svccfg.c
    )

# For our Flex and Lemon files:
target_include_directories(svccfg PRIVATE ${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR})

target_link_libraries (svccfg s16 s16db s16rpc uthash ucl ${LIBKQUEUE_LIBRARY} ${Readline_LIBRARY})

install(TARGETS svccfg RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR})

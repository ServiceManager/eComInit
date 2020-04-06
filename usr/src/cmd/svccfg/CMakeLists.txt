project (svccfg)

FlexComp(svccfg.l)
LemonComp(svccfg.y)

add_executable (svccfg
    ${PROJECT_BINARY_DIR}/svccfg.tab.c ${PROJECT_BINARY_DIR}/svccfg.y.c svccfg.c
    )

# For our Flex and Lemon files:
target_include_directories(svccfg PRIVATE ${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR})

target_link_libraries (svccfg s16 uthash ucl Kqueue::Kqueue Readline::Readline)

install(TARGETS svccfg RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR})

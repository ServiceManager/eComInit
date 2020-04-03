project (svcadm)

FlexComp(svcadm.l)
LemonComp(svcadm.y)

add_executable (svcadm
    ${PROJECT_BINARY_DIR}/svcadm.tab.c ${PROJECT_BINARY_DIR}/svcadm.y.c svcadm.c)

# For our Flex and Lemon files:
target_include_directories(svcadm PRIVATE ${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR})

target_link_libraries (svcadm s16 s16db s16rpc ucl ${LIBKQUEUE_LIBRARY} Readline::Readline)

install(TARGETS svcadm RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR})

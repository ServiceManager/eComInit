cmake_minimum_required (VERSION 2.8)

file(GLOB METHODS "method/*")
install(PROGRAMS ${METHODS}
          DESTINATION "${CMAKE_INSTALL_SYSCONFDIR}/s16/method")

file(GLOB MANIFESTS "manifest/*.ucl")
install(FILES ${MANIFESTS}
          DESTINATION "${CMAKE_INSTALL_SYSCONFDIR}/s16/L1manifest")
cmake_minimum_required (VERSION 2.8)

file(GLOB METHODS "method/*")
install(FILES ${METHODS}
          DESTINATION "lib/s16/method")

file(GLOB MANIFESTS "manifest/*.ucl")
install(FILES ${MANIFESTS}
          DESTINATION "etc/s16/manifest")